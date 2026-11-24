#!/usr/bin/env python3

import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass

EXTRA_RE = re.compile(r"[<>=:]")  # Remove version/description


@dataclass(slots=True)
class PackageInfo:
    directory: str
    deps: list[str]
    processed: bool = False


def run(*args: str, **kwargs) -> str:
    return subprocess.check_output(args, text=True, encoding="utf-8", **kwargs).strip()


def get_toplevel() -> str:
    path = run("git", "rev-parse", "--show-toplevel")
    return run("cygpath", "-m", path)


def list_changes(*git_args: str) -> list[str]:
    out = run("git", "log", *git_args, "upstream/master..").splitlines()
    out += run("git", "log", *git_args, "HEAD^..").splitlines()
    return list(dict.fromkeys(x.split("::")[-1] for x in sorted(out)))


def list_packages() -> list[str]:
    changes = list_changes("--pretty=format:", "--name-only")

    # if: block resolving from path like foo/bar/PKGBUILD to foo
    # and failing on: source "foo/PKGBUILD"
    return [
        x.split("/")[0]
        for x in changes
        if x.count("/") == 1 and x.endswith("/PKGBUILD") and os.path.exists(x)
    ]


def get_pkginfo(package: str) -> tuple[PackageInfo, tuple[str, ...]]:
    props = ("depends", "makedepends", "pkgname", "provides")

    # ignore potential stdout noise
    script = f'source "{package}/PKGBUILD" >/dev/null\n'

    for prop in props:
        script += f"echo \"${{{prop}[@]}}\" && printf '\\0'\n"

    shell = os.environ.get("SHELL", "bash")

    # overrides for MSYS repository only
    env = {
        **os.environ,
        "MINGW_PACKAGE_PREFIX": os.environ.get("MINGW_PACKAGE_PREFIX") or "mingw-w64",
    }
    results = run(shell, "-c", script, env=env).split("\0")[:-1]

    info = {
        prop: [EXTRA_RE.split(x, 1)[0] for x in res.strip().split() if x]
        for prop, res in zip(props, results, strict=True)
    }

    deps = sorted({*info["depends"], *info["makedepends"]})
    pkg = PackageInfo(package, deps)
    aliases = (package, *info["pkgname"], *info["provides"])
    return pkg, aliases


def get_build_order(
    packages: list[str],
    toadd: dict[str, PackageInfo] | None = None,
    ordered: list[str] | None = None,
) -> list[str]:
    if toadd is None or ordered is None:
        toadd, ordered = {}, []
        with ThreadPoolExecutor() as pool:
            for pkg, aliases in pool.map(get_pkginfo, packages):
                for alias in aliases:
                    toadd[alias] = pkg

    for package in packages:
        if package not in toadd:
            continue

        pkg = toadd[package]
        if pkg.directory in ordered:
            continue
        if pkg.processed:
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
