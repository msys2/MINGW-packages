#!/usr/bin/env python3

import argparse
import logging
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import textwrap
from collections.abc import Generator
from contextlib import contextmanager
from functools import cache
from pathlib import Path
from sys import stdout

logging.basicConfig(level=logging.INFO, format="%(message)s")
logger = logging.getLogger(__name__)

RDEPS_REGEX = re.compile(
    r"Repository\s*: \s*[\n\S\s]*\nRequired By\s*:\s*(?P<rdeps>[\s\S]*)Optional For\s*:\s*"
)
OPTIONS_REGEX = re.compile(r"options=\((?P<options>[\S ]*)\)")
REPO_ROOT = Path(__file__).parent.parent


def convert_win2unix_path(winpath: Path | str) -> str:
    p = subprocess.run(
        ["cygpath", "-u", str(winpath)],
        capture_output=True,
        text=True,
        check=True,
    )
    return p.stdout.strip()


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
    with conf.open("r", encoding="utf-8") as h:
        text = h.read()
        uri = repo_dir.as_uri()
        if uri not in text:
            with open(conf, "w", encoding="utf-8") as h2:
                h2.write(
                    textwrap.dedent(
                        f"""\
                        [ci]
                        Server={uri}
                        SigLevel=Never
                        """
                    )
                )
                h2.write(text)
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
    assert repo_path.exists()


@contextmanager
def install_package(pkg: list[str] | Path) -> Generator[None]:
    if isinstance(pkg, Path):
        command = [
            "pacman",
            "--noprogressbar",
            "--refresh",
            "--sync",
            "--noconfirm",
            "--needed",
            "mingw-w64-ucrt-x86_64-python-pip",
            get_pkg_name(pkg),
        ]
        uninstall_command = [
            "pacman",
            "--noprogressbar",
            "--remove",
            "--noconfirm",
            "-cns",
            get_pkg_name(pkg),
        ]
    else:
        if len(pkg) != 0:
            command = [
                "pacman",
                "--sync",
                "--refresh",
                "--noconfirm",
                "--needed",
                "mingw-w64-ucrt-x86_64-python-pip",
                *pkg,
            ]
            uninstall_command = [
                "pacman",
                "--noprogressbar",
                "--remove",
                "--noconfirm",
                "-cns",
                *pkg,
            ]
        else:
            command = uninstall_command = ["true"]
    with gha_group(f"Installing: {pkg}"):
        ret = subprocess.run(command, check=False)
    if ret.returncode != 0:
        logger.error(
            "::error :: %s failed to run. Returned return code: %s",
            command,
            ret.returncode,
        )
        logger.info("::error :: Failed to Install, skipping the test.")
    yield
    with gha_group(f"Uninstalling: {pkg}"):
        subprocess.run(uninstall_command, check=False)


def get_rdeps(pkg: str) -> list[str]:
    with gha_group("Finding Reverse Dependency"):
        p = subprocess.run(
            ["pacman", "-Sii", pkg],
            capture_output=True,
            text=True,
            check=False,
        )
        if p.returncode != 0:
            logger.info("Looks like a new package.")
            p = subprocess.run(
                ["pacman", "-Qi", pkg],
                capture_output=True,
                text=True,
                check=False,
            )
        logger.info("Installing rdeps of: %s", pkg)
        logger.info("Pkgname: %s", pkg)
        logger.info("Installing Package.")
        ret = p.stdout
        rdeps_re = RDEPS_REGEX.search(ret)
        if rdeps_re:
            pkgs_str = rdeps_re.group("rdeps")
            pkgs = [i.strip() for i in pkgs_str.split() if i != "None"]
            logger.info("Rdeps parsed: %s", pkgs)
            return pkgs
        else:
            raise ValueError(f"Unable to parse Pacman output: {ret}")


def run_pip_check(pkg: str) -> None:
    p = subprocess.run(
        [
            sys.executable,
            "-m",
            "pip",
            "check",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if p.returncode != 0:
        logger.error("Pip Check Failed for %s", pkg)
        logger.error("Running `pip check` didn't suceed. Maybe missing dependencies?")
        logger.error("Here is what it failed.")
        logger.error(p.stdout)
        logger.error("::error :: %s", p.stderr)
        sys.exit(1)
    logger.info("All dependencies are correct satisfied for %s", pkg)


@contextmanager
def gha_group(title: str) -> Generator[None]:
    print(f"\n::group::{title}")
    stdout.flush()
    try:
        yield
    finally:
        print("::endgroup::")
        stdout.flush()


def gha_error(filename: Path, title: str, message: str) -> None:
    print(
        f"::error file={filename.relative_to(REPO_ROOT).as_posix()},title={title}::{message}"
    )
    stdout.flush()


def get_pkg_name(pkgloc: Path) -> str:
    return "-".join(pkgloc.name.split("-")[:-3])


def get_pkgbuild_file(repo_dir: Path, pkg_name: str) -> Path:
    db = get_pacdb_database(repo_dir)
    pkg = db.get_pkg(pkg_name)
    if pkg is None:
        raise ValueError(f"Unable to find {pkg_name} in the database.")
    return REPO_ROOT / pkg.base / "PKGBUILD"


def check_strip_disabled_for_pkg(repo_dir: Path, pkg_name: str) -> bool:
    pkgbuild_file = get_pkgbuild_file(repo_dir, pkg_name)
    if not pkgbuild_file.exists():
        raise ValueError(
            f"Unable to find PKGBUILD for {pkg_name}: tried {pkgbuild_file}"
        )
    with open(pkgbuild_file, "r", encoding="utf-8") as h:
        text = h.read()
        match = OPTIONS_REGEX.search(text)
        if match and "!strip" in match.group("options"):
            return True
    return False


def check_whether_using_python_installer(repo_dir: Path, pkg_name: str) -> bool:
    db = get_pacdb_database(repo_dir)
    pkg = db.get_pkg(pkg_name)
    if pkg is None:
        raise ValueError(f"Unable to find {pkg_name} in the database.")
    return "mingw-w64-ucrt-x86_64-python-installer" in pkg.makedepends


def check_whether_files_in_bin_folder(repo_dir: Path, pkg_name: str) -> bool:
    db = get_pacdb_database(repo_dir)
    pkg = db.get_pkg(pkg_name)
    if pkg is None:
        raise ValueError(f"Unable to find {pkg_name} in the database.")
    return any(i.startswith("ucrt64/bin") and i.endswith(".exe") for i in pkg.files)


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

    print("Strip option check is done.")

    for pkgloc in pkgs:
        with install_package(pkgloc):
            pkgname = get_pkg_name(pkgloc)
            with gha_group(f"Pip Output (without reverse dependencies): {pkgloc.name}"):
                run_pip_check(pkgname)
            rdeps = get_rdeps(pkgname)
            with (
                install_package(rdeps),
                gha_group(f"Pip Output (with reverse dependencies): {pkgloc.name}"),
            ):
                run_pip_check(pkgname)


def check_whether_we_should_run(artifacts_dir: Path) -> bool:
    if not artifacts_dir.exists():
        return False
    for pkgloc in artifacts_dir.glob("*.pkg.tar.*"):
        pkgname = get_pkg_name(pkgloc)
        if "-python-" in pkgname:
            return True
        elif pkgname == "mingw-w64-ucrt-x86_64-python":
            return False
    # now this is complicated
    # need to find file listing in the package.
    # try using zstd, it's installed by default on GHA machines
    for pkgloc in artifacts_dir.glob("*.pkg.tar.*"):
        with tempfile.TemporaryDirectory() as tempdir:
            shutil.copy(pkgloc, tempdir)
            subprocess.run(
                ["zstd", "-d", str(Path(tempdir, pkgloc.name))],
                check=True,
                cwd=tempdir,
            )
            with tarfile.open(Path(tempdir, pkgloc.stem), mode="r") as tar:
                for mem in tar.getmembers():
                    if "ucrt64/lib/python" in mem.name:
                        return True
    return False


if __name__ == "__main__":
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
    args = parser.parse_args()
    artifacts_dir = args.directory.resolve()

    if args.run:
        res = check_whether_we_should_run(artifacts_dir)

        output = f"run={'true' if res else 'false'}"

        if github_output := os.environ.get("GITHUB_OUTPUT"):
            with open(github_output, "a", encoding="utf-8") as f:
                print(output, file=f)
        else:
            print(output)

        sys.exit(0)
    else:
        main(artifacts_dir)
