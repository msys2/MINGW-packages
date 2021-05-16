#!/usr/bin/env python3

import argparse
import logging
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import typing
from pathlib import Path

logging.basicConfig(level=logging.INFO, format="%(message)s")
logger = logging.getLogger(__file__)

ARTIFACTS_LOCATION = Path("C:/_/artifacts")
RDEPS_REGEX = re.compile(
    r"Required By\s*:\s*(?P<rdeps>[\s\-_A-za-z1-9]*)Optional For\s*:\s*None"
)


def convert_win2unix_path(winpath: typing.Union[Path, str]):
    p = subprocess.run(
        ["cygpath", "-u", str(winpath)],
        capture_output=True,
        text=True,
        check=True,
    )
    return p.stdout.strip()


def install_package(pkg: typing.Union[Path, str], local: bool) -> None:
    logger.info("Installing %s", pkg)
    if local:
        command = [
            "pacman",
            "--noprogressbar",
            "--upgrade",
            "--noconfirm",
            convert_win2unix_path(pkg),
        ]
    else:
        command = [
            "pacman",
            "-S",
            "--noconfirm",
            pkg,
        ]
    subprocess.run(
        command,
        check=True,
    )


def get_rdeps(pkg: str) -> typing.List[str]:
    p = subprocess.run(
        ["pacman", "-Sii", pkg],
        capture_output=True,
        text=True,
        check=True,
    )
    ret = p.stdout
    rdeps_re = RDEPS_REGEX.search(ret)
    if rdeps_re:
        pkgs_str = rdeps_re.group("rdeps")
        pkgs = [i.strip() for i in pkgs_str.split() if i != "None"]
        return pkgs
    else:
        raise ValueError("Unable to parse Pacman output")


def run_pip_check(pkg: str) -> None:
    p = subprocess.run(
        [sys.executable, "-m", "pip", "check"],
        capture_output=True,
        text=True,
    )
    if p.returncode != 0:
        logger.error("Pip Check Failed for %s", pkg)
        logger.error("Running `pip check` didn't suceed. Maybe missing dependencies?")
        logger.error("Here is what it failed.")
        logger.error(p.stdout)
        logger.error(p.stderr)
        sys.exit(1)
    logger.info("All dependencies are correct satisfied for %s", pkg)


def main():
    print("::group:: Outputs")
    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        pkgname = "-".join(pkgloc.name.split("-")[:-3])
        logger.info("Pkgname: %s", pkgname)
        logger.info("Installing Package.")
        install_package(pkgloc, local=True)
        rdeps = get_rdeps(pkgname)
        for dep in rdeps:
            install_package(dep, local=False)
        print("::endgroup::")
        run_pip_check(pkgname)


def check_whether_we_should_run() -> bool:
    if not ARTIFACTS_LOCATION.exists():
        return False
    for pkgloc in ARTIFACTS_LOCATION.glob("*.pkg.tar.*"):
        if "-python-" in pkgloc.name:
            return True
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
