#!/usr/bin/env python3

import argparse
import logging
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import textwrap
import typing
from contextlib import contextmanager
from pathlib import Path
from sys import stdout

logging.basicConfig(level=logging.INFO, format="%(message)s")
logger = logging.getLogger(__file__)

TEMP_REPO_PATH = ARTIFACTS_LOCATION = Path("C:/_/artifacts")
RDEPS_REGEX = re.compile(r"Repository\s*: mingw64[\n\S\s]*\nRequired By\s*:\s*(?P<rdeps>[\s\S]*)Optional For\s*:\s*")


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


def get_pkg_name(pkgloc: Path):
    return "-".join(pkgloc.name.split("-")[:-3])


def main() -> None:
    create_and_add_to_repo(list(ARTIFACTS_LOCATION.glob("*.pkg.tar.*")))
    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        with install_package(pkgloc):
            pkgname = get_pkg_name(pkgloc)
            rdeps = get_rdeps(pkgname)
            with install_package(rdeps):
                with gha_group(f"Pip Output: {pkgloc.name}"):
                    run_pip_check(pkgname)


def check_whether_we_should_run() -> bool:
    if not ARTIFACTS_LOCATION.exists():
        return False
    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        pkgname = "-".join(pkgloc.name.split("-")[:-3])
        if "-python-" in pkgname:
            return True
        elif pkgname == "mingw-w64-x86_64-python":
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
                        if "mingw64/lib/python" in mem.name:
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
    args = parser.parse_args()
    if args.run:
        res = check_whether_we_should_run()
        if res:
            print("::set-output name=run::true")
        else:
            print("::set-output name=run::false")
        sys.exit(0)
    else:
        main()
