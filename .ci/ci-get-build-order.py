#!/usr/bin/env python3

from dataclasses import dataclass
from multiprocessing.pool import ThreadPool

import os
import re
import subprocess
import sys


@dataclass
class PackageInfo:
    directory: str
    deps: list
    processed: bool


def run(*args, **kwargs):
    return subprocess.check_output(args, **kwargs).decode("utf-8").strip()


def get_toplevel():
    path = run("git", "rev-parse", "--show-toplevel")
    return run("cygpath", "-m", path)


def list_changes(*git_args):
    out = run("git", "log", *git_args, "upstream/master..").splitlines()
    out += run("git", "log", *git_args, "HEAD^..").splitlines()
    return list(dict.fromkeys(x.split("::")[-1] for x in sorted(out)))


def list_packages():
    changes = list_changes("--pretty=format:", "--name-only")
    return [
        x.split("/")[0]
        for x in changes
        if x.endswith("/PKGBUILD") and os.path.exists(x)
    ]


def get_pkginfo(package, packageset):
    props = ["depends", "makedepends", "pkgname", "provides"]
    script = f'source "{package}/PKGBUILD"\n'
    for prop in props:
        script += f"echo \"${{{prop}[@]}}\" && printf '\\0'\n"

    shell = os.environ.get("SHELL", "bash")
    env = {**os.environ, "MINGW_PACKAGE_PREFIX": "mingw-w64"}
    results = run(shell, "-c", script, env=env).split("\0")[:-1]
    assert len(props) == len(results), "Length of props matches results"

    info = {}
    extra_re = re.compile(r"[<>=:]")  # Remove version/description
    for prop, res in zip(props, results):
        res = [extra_re.split(x, 1)[0] for x in res.strip().split() if x]
        info[prop] = res

    deps = sorted(set((*info["depends"], *info["makedepends"])))
    pkg = PackageInfo(package, deps, False)
    for alias in (package, *info["pkgname"], *info["provides"]):
        packageset[alias] = pkg
    return pkg


def get_build_order(packages, toadd=None, ordered=None):
    if toadd is None:
        toadd, ordered = {}, []
        ThreadPool(os.cpu_count()).map(lambda x: get_pkginfo(x, toadd), packages)

    for package in packages:
        if package not in toadd:
            continue

        pkg = toadd[package]
        if pkg.directory in ordered:
            continue
        elif pkg.processed:
            print("warning: dependency cycle detected on", package, file=sys.stderr)
            continue
        pkg.processed = True

        get_build_order(pkg.deps, toadd, ordered)
        ordered.append(pkg.directory)
    return ordered


if __name__ == "__main__":
    os.chdir(get_toplevel())
    packages = "\n".join(get_build_order(list_packages()))
    sys.stdout.buffer.write(packages.encode("utf-8"))  # Prevent CRLF newlines
