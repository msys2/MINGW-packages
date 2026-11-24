#!/usr/bin/env python3

import argparse
import logging
import os
import re
import subprocess
import sys
import tarfile
import textwrap
from collections.abc import Generator, Sequence
from contextlib import contextmanager
from enum import Enum, auto
from functools import cache
from pathlib import Path
from typing import NamedTuple


class InstallStatus(Enum):
    OK = auto()
    CONFLICT = auto()
    FAILED = auto()


class InstallResult(NamedTuple):
    status: InstallStatus
    output: str = ""


logging.basicConfig(level=logging.INFO, format="%(message)s")
logger = logging.getLogger(__name__)

RDEPS_REGEX = re.compile(
    r"^Required By\s*:\s*(?P<rdeps>.*?)\nOptional For\s*:",
    re.MULTILINE | re.DOTALL,
)
OPTIONS_REGEX = re.compile(r"options=\((?P<options>[\S ]*)\)")
CONFLICT_REGEX = re.compile(
    r"^::\s+(\S+)\s+and\s+(\S+)\s+are in conflict",
    re.MULTILINE,
)

REPO_ROOT = Path(__file__).parent.parent
MINGW_PREFIX = "mingw-w64-ucrt-x86_64"
ENV_PREFIX = "ucrt64"
PYTHON_PKG = f"{MINGW_PREFIX}-python"
PIP_PKG = f"{MINGW_PREFIX}-python-pip"
INSTALLER_PKG = f"{MINGW_PREFIX}-python-installer"


def convert_win2unix_path(winpath: Path | str) -> str:
    p = subprocess.run(
        ["cygpath", "-u", str(winpath)],
        capture_output=True,
        text=True,
        check=True,
    )
    return p.stdout.strip()


@cache
def get_msys2_root() -> str:
    p = subprocess.run(
        ["cygpath", "-w", "/"],
        capture_output=True,
        text=True,
        check=True,
    )
    return p.stdout.strip()


@cache
def get_pacdb_database(repo_dir: Path):
    import pacdb

    return pacdb.Database("ci", repo_dir / "ci.files.tar.gz")


def create_and_add_to_repo(repo_dir: Path, pkgs: list[Path]) -> None:
    repo_dir.mkdir(exist_ok=True)
    conf = Path(get_msys2_root(), "etc/pacman.conf")
    text = conf.read_text(encoding="utf-8")
    uri = repo_dir.as_uri()
    if uri not in text:
        header = textwrap.dedent(
            f"""\
            [ci]
            Server={uri}
            SigLevel=Never
            """
        )
        conf.write_text(header + text, encoding="utf-8")

    repo_path = repo_dir / "ci.db.tar.gz"
    with gha_group("Create temporary Repository:"):
        subprocess.run(
            [
                "sh",
                "repo-add",
                convert_win2unix_path(repo_path),
                *[convert_win2unix_path(pkg) for pkg in pkgs],
            ],
            check=True,
            cwd=repo_dir,
        )
    if not repo_path.exists():
        raise FileNotFoundError(f"Expected repository database at {repo_path}")


def refresh_pacman_db() -> None:
    with gha_group("Refreshing pacman databases"):
        subprocess.run(
            ["pacman", "--noprogressbar", "--sync", "--refresh", "--noconfirm"],
            check=True,
        )


@contextmanager
def install_package(pkgs: Sequence[str]) -> Generator[InstallResult]:
    if not pkgs:
        yield InstallResult(InstallStatus.OK)
        return

    command = [
        "pacman",
        "--noprogressbar",
        "--sync",
        "--noconfirm",
        "--needed",
        PIP_PKG,
        *pkgs,
    ]
    uninstall_command = [
        "pacman",
        "--noprogressbar",
        "--remove",
        "--noconfirm",
        "-cns",
        *pkgs,
    ]

    with gha_group(f"Installing: {', '.join(pkgs)}"):
        ret = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        output = ret.stdout or ""
        if output:
            print(output, end="" if output.endswith("\n") else "\n", flush=True)

    if ret.returncode != 0:
        if "conflicting dependencies" in output:
            logger.warning(
                "Package conflict detected while installing %s.", ", ".join(pkgs)
            )
            yield InstallResult(InstallStatus.CONFLICT, output)
            return

        logger.error(
            "::error :: %s failed with return code %s.",
            command,
            ret.returncode,
        )
        yield InstallResult(InstallStatus.FAILED, output)
        return

    try:
        yield InstallResult(InstallStatus.OK, output)
    finally:
        with gha_group(f"Uninstalling: {', '.join(pkgs)}"):
            subprocess.run(uninstall_command, check=False)


def find_conflicting_rdep(output: str, rdeps: Sequence[str]) -> str | None:
    """Return one package from rdeps named in pacman's conflict output, if any."""
    rdep_set = set(rdeps)
    for match in CONFLICT_REGEX.finditer(output):
        for token in (match.group(1), match.group(2)):
            if token in rdep_set:
                return token
            for pkg in rdeps:
                if token.startswith(f"{pkg}-"):
                    return pkg
    return None


def get_rdeps(pkg: str) -> list[str]:
    with gha_group(f"Finding reverse dependencies for {pkg}"):
        p = subprocess.run(
            ["pacman", "-Sii", pkg],
            capture_output=True,
            text=True,
            check=False,
        )
        if p.returncode != 0:
            logger.info("%s not found in sync db; querying local db.", pkg)
            p = subprocess.run(
                ["pacman", "-Qi", pkg],
                capture_output=True,
                text=True,
                check=False,
            )
        if p.returncode != 0:
            raise RuntimeError(f"pacman failed for {pkg}: {p.stderr.strip()}")

        match = RDEPS_REGEX.search(p.stdout)
        if not match:
            raise ValueError(f"Unable to parse pacman output:\n{p.stdout}")

        pkgs = [name for name in match.group("rdeps").split() if name != "None"]
        logger.info("Reverse dependencies for %s: %s", pkg, pkgs)
        return pkgs


def check_rdeps_batch(pkgname: str, rdeps: Sequence[str]) -> bool:
    """Install and `pip check` rdeps, peeling off conflicting packages automatically."""
    if not rdeps:
        return True

    with install_package(rdeps) as result:
        if result.status is InstallStatus.OK:
            with gha_group(f"Pip Output (with rdeps {', '.join(rdeps)}): {pkgname}"):
                return run_pip_check(pkgname)

        if result.status is InstallStatus.CONFLICT and len(rdeps) > 1:
            culprit = find_conflicting_rdep(result.output, rdeps)
            if culprit is not None:
                rest = [p for p in rdeps if p != culprit]
                logger.info(
                    "Deferring conflicting rdep %s; retrying %d others.",
                    culprit,
                    len(rest),
                )
                rest_ok = check_rdeps_batch(pkgname, rest)
                culprit_ok = check_rdeps_batch(pkgname, [culprit])
                return rest_ok and culprit_ok

            # Transitive conflict fallback: stride split separates adjacent names
            even_batch = rdeps[::2]
            odd_batch = rdeps[1::2]
            logger.info(
                "Transitive conflict in %d rdeps; stride-splitting into batches of %d and %d.",
                len(rdeps),
                len(even_batch),
                len(odd_batch),
            )
            even_ok = check_rdeps_batch(pkgname, even_batch)
            odd_ok = check_rdeps_batch(pkgname, odd_batch)
            return even_ok and odd_ok

        # Single-package conflict (e.g. conflicts with pkgname itself) or download/other error
        logger.error(
            "::error :: Could not install rdeps %s for %s; dependency check incomplete.",
            list(rdeps),
            pkgname,
        )
        return False


def run_pip_check(pkg: str) -> bool:
    p = subprocess.run(
        [sys.executable, "-m", "pip", "check"],
        capture_output=True,
        text=True,
        check=False,
    )
    if p.returncode != 0:
        logger.error("`pip check` failed for %s (possible missing dependencies).", pkg)
        if p.stdout.strip():
            logger.error("::error :: %s", p.stdout.strip())
        if p.stderr.strip():
            logger.error("::error :: %s", p.stderr.strip())
        return False

    logger.info("All dependencies are satisfied for %s.", pkg)
    return True


@contextmanager
def gha_group(title: str) -> Generator[None]:
    print(f"\n::group::{title}", flush=True)
    try:
        yield
    finally:
        print("::endgroup::", flush=True)


def gha_error(filename: Path, title: str, message: str) -> None:
    print(
        f"::error file={filename.relative_to(REPO_ROOT).as_posix()},title={title}::{message}",
        flush=True,
    )


def get_pkg_name(pkgloc: Path) -> str:
    return "-".join(pkgloc.name.split("-")[:-3])


def get_pkg(repo_dir: Path, pkg_name: str):
    db = get_pacdb_database(repo_dir)
    pkg = db.get_pkg(pkg_name)
    if pkg is None:
        raise ValueError(f"Unable to find {pkg_name} in the database.")
    return pkg


def get_pkgbuild_file(repo_dir: Path, pkg_name: str) -> Path:
    return REPO_ROOT / get_pkg(repo_dir, pkg_name).base / "PKGBUILD"


def check_strip_disabled_for_pkg(repo_dir: Path, pkg_name: str) -> bool:
    pkgbuild_file = get_pkgbuild_file(repo_dir, pkg_name)
    if not pkgbuild_file.exists():
        raise FileNotFoundError(
            f"Unable to find PKGBUILD for {pkg_name}: tried {pkgbuild_file}"
        )
    text = pkgbuild_file.read_text(encoding="utf-8")
    match = OPTIONS_REGEX.search(text)
    return bool(match and "!strip" in match.group("options"))


def check_whether_using_python_installer(repo_dir: Path, pkg_name: str) -> bool:
    pkg = get_pkg(repo_dir, pkg_name)
    return INSTALLER_PKG in pkg.makedepends


def check_whether_files_in_bin_folder(repo_dir: Path, pkg_name: str) -> bool:
    pkg = get_pkg(repo_dir, pkg_name)
    return any(
        i.startswith(f"{ENV_PREFIX}/bin/") and i.endswith(".exe") for i in pkg.files
    )


def main(artifacts_dir: Path) -> None:
    pkgs = list(artifacts_dir.glob("*.pkg.tar.*"))
    create_and_add_to_repo(artifacts_dir, pkgs)

    # iterate through all the packages and check if they are using python-installer
    # in which case iterate through the files in the package and check if there's anything
    # in the ucrt64/bin folder. If there is, then check whether strip is disabled.
    exit_code = 0
    for pkg_loc in pkgs:
        pkg_name = get_pkg_name(pkg_loc)
        if (
            check_whether_using_python_installer(artifacts_dir, pkg_name)
            and check_whether_files_in_bin_folder(artifacts_dir, pkg_name)
            and not check_strip_disabled_for_pkg(artifacts_dir, pkg_name)
        ):
            gha_error(
                get_pkgbuild_file(artifacts_dir, pkg_name),
                "Strip option is not disabled",
                f"Package `{pkg_name}` is using `python-installer` and has executables "
                "but strip option is not disabled in the PKGBUILD.",
            )
            exit_code = 1

    if exit_code != 0:
        sys.exit(exit_code)

    logger.info("Strip option check is done.")
    refresh_pacman_db()

    for pkgloc in pkgs:
        pkgname = get_pkg_name(pkgloc)
        with install_package([pkgname]) as result:
            if result.status is not InstallStatus.OK:
                logger.error(
                    "::error :: Could not install %s; dependency check failed.",
                    pkgname,
                )
                exit_code = 1
                continue

            with gha_group(f"Pip Output (without reverse dependencies): {pkgloc.name}"):
                if not run_pip_check(pkgname):
                    exit_code = 1

            rdeps = get_rdeps(pkgname)
            if not check_rdeps_batch(pkgname, rdeps):
                exit_code = 1

    if exit_code != 0:
        sys.exit(exit_code)


def check_whether_we_should_run(artifacts_dir: Path) -> bool:
    if not artifacts_dir.is_dir():
        return False

    pkgs = list(artifacts_dir.glob("*.pkg.tar.*"))
    candidates: list[Path] = []

    for pkgloc in pkgs:
        pkgname = get_pkg_name(pkgloc)
        if "-python-" in pkgname:
            return True
        if pkgname == PYTHON_PKG:
            continue
        candidates.append(pkgloc)

    for pkgloc in candidates:
        with tarfile.open(pkgloc, mode="r:*") as tar:
            if any(f"{ENV_PREFIX}/lib/python" in name for name in tar.getnames()):
                return True

    return False


def cli(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="After install checks.")
    parser.add_argument(
        "--whether-to-run",
        help="Check whether to run.",
        dest="run",
        action="store_true",
    )
    parser.add_argument(
        "directory",
        help="Directory containing the artifacts.",
        type=Path,
    )
    args = parser.parse_args(argv)
    artifacts_dir = args.directory.resolve()

    if args.run:
        res = check_whether_we_should_run(artifacts_dir)

        output = f"run={'true' if res else 'false'}"

        if github_output := os.environ.get("GITHUB_OUTPUT"):
            with open(github_output, "a", encoding="utf-8") as f:
                print(output, file=f)
        else:
            print(output)
        return 0

    main(artifacts_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(cli())
