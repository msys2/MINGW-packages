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
import typing
from contextlib import contextmanager
from functools import cache
from pathlib import Path
from sys import stdout

try:
    import pacdb
except ImportError:
    pacdb = None

logging.basicConfig(level=logging.INFO, format="%(message)s")
logger = logging.getLogger(__file__)

TEMP_REPO_PATH = ARTIFACTS_LOCATION = None
RDEPS_REGEX = re.compile(r"Repository\s*: \s*[\n\S\s]*\nRequired By\s*:\s*(?P<rdeps>[\s\S]*)Optional For\s*:\s*")
OPTIONS_REGEX = re.compile(r"options=\((?P<options>[\S ]*)\)")
REPO_ROOT = Path(__file__).parent.parent


def convert_win2unix_path(winpath: typing.Union[Path, str]):
    p = subprocess.run(
        ["cygpath", "-u", str(winpath)],
        capture_output=True,
        text=True,
        check=True,
    )
    return p.stdout.strip()


def get_msys2_root():
    p = subprocess.run(
        ["cygpath", "-w", "/"],
        capture_output=True,
        text=True,
        check=True,
    )
    return p.stdout.strip()


@cache
def get_pacdb_database():
    return pacdb.Database("ci", TEMP_REPO_PATH / "ci.files.tar.gz")


def create_and_add_to_repo(pkgs: typing.List[Path]) -> None:
    TEMP_REPO_PATH.mkdir(exist_ok=True)
    conf = Path(get_msys2_root(), "etc/pacman.conf")
    with conf.open("r", encoding="utf-8") as h:
        text = h.read()
        uri = TEMP_REPO_PATH.as_uri()
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
    repo_path = TEMP_REPO_PATH / "ci.db.tar.gz"
    with gha_group("Create temporary Repository:"):
        subprocess.run(
            [
                "sh",
                "repo-add",
                convert_win2unix_path(repo_path),
                *[convert_win2unix_path(pkg) for pkg in pkgs],
            ],
            check=True,
            cwd=TEMP_REPO_PATH,
        )
    assert repo_path.exists()


@contextmanager
def install_package(pkg: typing.Union[typing.List[str], Path]) -> typing.Generator:
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
            command = uninstall_command = ['true']
    with gha_group(f"Installing: {pkg}"):
        ret = subprocess.run(command)
    if ret.returncode != 0:
        logger.error(
            "::error :: %s failed to run. Returned return code: %s", command, ret.returncode
        )
        logger.info("::error :: Failed to Install, skipping the test.")
    yield
    with gha_group(f"Uninstalling: {pkg}"):
        ret = subprocess.run(uninstall_command)


def get_rdeps(pkg: str) -> typing.List[str]:
    with gha_group(f"Finding Reverse Dependency"):
        p = subprocess.run(
            ["pacman", "-Sii", pkg],
            capture_output=True,
            text=True,
        )
        if p.returncode != 0:
            logger.info("Looks like a new package.")
            p = subprocess.run(
                ["pacman", "-Qi", pkg],
                capture_output=True,
                text=True,
            )
        logger.info(f"Installing rdeps of: {pkg}")
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
    )
    if p.returncode != 0:
        logger.error("Pip Check Failed for %s", pkg)
        logger.error("Running `pip check` didn't suceed. Maybe missing dependencies?")
        logger.error("Here is what it failed.")
        logger.error(p.stdout)
        logger.error("::error :: %s",p.stderr)
        sys.exit(1)
    logger.info("All dependencies are correct satisfied for %s", pkg)


@contextmanager
def gha_group(title: str) -> typing.Generator:
    print(f"\n::group::{title}")
    stdout.flush()
    try:
        yield
    finally:
        print("::endgroup::")
        stdout.flush()


def gha_error(filename: Path, title: str, message: str) -> None:
    print(f"::error file={filename.relative_to(REPO_ROOT).as_posix()},title={title}::{message}")
    stdout.flush()


def get_pkg_name(pkgloc: Path):
    return "-".join(pkgloc.name.split("-")[:-3])


def get_pkgbuild_file(pkg_name: str) -> bool:
    db = get_pacdb_database()
    pkg = db.get_pkg(pkg_name)
    pkg_base = pkg.base
    return REPO_ROOT / pkg_base / "PKGBUILD"


def check_strip_disabled_for_pkg(pkg_name: str) -> bool:
    pkgbuild_file = get_pkgbuild_file(pkg_name)
    if not pkgbuild_file.exists():
        raise ValueError(
            f"Unable to find PKGBUILD for {pkg_name}: tried {pkgbuild_file}"
        )
    with open(pkgbuild_file, "r", encoding="utf-8") as h:
        text = h.read()
        # find for options using OPTIONS_REGEX
        options = OPTIONS_REGEX.search(text)
        if options:
            options = options.group("options")
            if "!strip" in options:
                return True
    return False


def check_whether_using_python_installer(pkg_name: str) -> bool:
    db = get_pacdb_database()
    pkg = db.get_pkg(pkg_name)
    if pkg is None:
        raise ValueError(f"Unable to find {pkg_name} in the database.")
    return "mingw-w64-ucrt-x86_64-python-installer" in pkg.makedepends


def check_whether_files_in_bin_folder(pkg_name: str) -> bool:
    db = get_pacdb_database()
    return any(i.startswith("ucrt64/bin") and i.endswith(".exe") for i in db.get_pkg(pkg_name).files)


def main() -> None:
    create_and_add_to_repo(list(ARTIFACTS_LOCATION.glob("*.pkg.tar.*")))

    # iterate through all the packages and check if they are using python-installer
    # in which case iterate through the files in the package and check if there's anything
    # in the ucrt64/bin folder. If there is, then check whether strip is disabled.
    exit_code = 0
    for pkg_loc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        pkg_name = get_pkg_name(pkg_loc)
        if check_whether_using_python_installer(pkg_name):
            if check_whether_files_in_bin_folder(pkg_name):
                if not check_strip_disabled_for_pkg(pkg_name):
                    gha_error(
                        get_pkgbuild_file(pkg_name),
                        "Strip option is not disabled",
                        f"Package `{pkg_name}` is using `python-installer` and has executables "
                        "but strip option is not disabled in the PKGBUILD.",
                    )
                    exit_code = 1

    if exit_code != 0:
        sys.exit(exit_code)
    
    print("Strip option check is done.")

    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        with install_package(pkgloc):
            pkgname = get_pkg_name(pkgloc)
            with gha_group(f"Pip Output (without reverse dependencies): {pkgloc.name}"):
                run_pip_check(pkgname)
            rdeps = get_rdeps(pkgname)
            with install_package(rdeps):
                with gha_group(f"Pip Output (with reverse dependencies): {pkgloc.name}"):
                    run_pip_check(pkgname)


def check_whether_we_should_run() -> bool:
    if not ARTIFACTS_LOCATION.exists():
        return False
    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        pkgname = "-".join(pkgloc.name.split("-")[:-3])
        if "-python-" in pkgname:
            return True
        elif pkgname == "mingw-w64-ucrt-x86_64-python":
            return False
    # now this is compilcated
    # need to find file listing in the package.
    # try using zstd, it's installed by default on GHA machines
    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        with tempfile.TemporaryDirectory() as tempdir:
            shutil.copy(pkgloc, tempdir)
            subprocess.run(
                ["zstd", "-d", str(Path(tempdir, pkgloc.name))],
                check=True,
                cwd=tempdir,
            )
            with open(Path(tempdir, pkgloc.stem), "rb") as file:
                with tarfile.open(fileobj=file, mode="r") as tar:
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
    TEMP_REPO_PATH = ARTIFACTS_LOCATION = args.directory.resolve()

    if args.run:
        res = check_whether_we_should_run()

        _op_file = sys.stdout
        _file_name = os.environ.get("GITHUB_OUTPUT")
        if _file_name:
            _op_file = open(_file_name, "a")
        if res:
            print("run=true", file=_op_file)
        else:
            print("run=false", file=_op_file)
        if _op_file is not sys.stdout:
            _op_file.close()

        sys.exit(0)
    else:
        main()
