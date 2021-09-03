#!/usr/bin/env python3

from dataclasses import dataclass
from multiprocessing.pool import ThreadPool

import os
import subprocess
import re


@dataclass
class PackageInfo:
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


def list_commits():
    return list_changes("--pretty=format:%ai::[%h] %s")


def list_packages():
    changes = list_changes("--pretty=format:", "--name-only")
    return [
        x.split("/")[0]
        for x in changes
        if x.endswith("/PKGBUILD") and os.path.exists(x)
    ]


def get_pkginfo(package, packageset):
    props = ["depends", "makedepends", "pkgname", "provides"]
    script = f'set -e && source "{package}/PKGBUILD"\n'
    for prop in props:
        script += f"echo \"${{{prop}[@]}}\" && printf '\\0'\n"

    shell = os.environ.get("SHELL", "bash")
    env = {"MINGW_PACKAGE_PREFIX": "mingw-w64"}
    results = run(shell, "-c", script, env=env).split("\0")[:-1]
    assert len(props) == len(results), "Length of props matches results"

    info = {}
    version_re = re.compile(r"[<>=]")
    for prop, res in zip(props, results):
        res = [x for x in res.strip().split() if x]
        if prop in ("depends", "makedepends"):
            res = [version_re.split(x)[0] for x in res]
        info[prop] = res

    deps = sorted(set(info["depends"]) | set(info["makedepends"]))
    pkg = PackageInfo(deps, False)
    for alias in set((package, *info["pkgname"], *info["provides"])):
        packageset[alias] = pkg
    return pkg


def get_build_order(packages, toadd=None, ordered=None):
    if toadd is None:
        toadd, ordered = {}, []
        ThreadPool(os.cpu_count()).map(lambda x: get_pkginfo(x, toadd), packages)

    for package in packages:
        if package in ordered or package not in toadd:
            continue

        pkg = toadd[package]
        if pkg.processed:
            print("WARN: Dependency cycle detected on", package)
            continue
        pkg.processed = True

        get_build_order(pkg.deps, toadd, ordered)
        ordered.append(package)
    return ordered


if __name__ == "__main__":
    os.chdir(get_toplevel())
    print(" ".join(get_build_order(list_packages())))
