#!/usr/bin/env python3
"""Strip debug info from chromium archives and objects pulled in by the
Qt6WebEngineCore.dll link, keeping debug only where requested.

Workflow
--------
1. Walk the build tree under ``--build-dir`` and parse every ``*.ninja`` to
   build two indexes:
     * ``obj_to_input``    - obj path -> primary input token (raw, as
                             written in the ninja file).
     * ``archive_to_objs`` - archive path -> [obj path, ...]
   Archive and obj paths are kept relative to the build root.
2. Build a *universe* of files to consider:
     * If one or more ``--rsp-file`` paths are given, parse each rsp and
       collect every ``.a``/``.o``/``.obj`` it references. This matches
       exactly what the Qt6WebEngineCore.dll link pulls in - including
       loose chromium objects passed via ``objects.rsp`` that no archive
       owns.
     * Otherwise, fall back to ``rglob('*.a')`` under ``--build-dir``.
3. For each ``.a`` archive, classify each member ``.o``:
     * Resolve its source(s) via the ninja index. Generated jumbo files
       (``gen/.../*_jumbo_*.cc``) are expanded through their ``#include``
       lines so the underlying chromium sources are visible.
     * The ``.o`` is *kept* if any of its sources starts with one of the
       ``--keep-prefix`` strings, or if the archive itself matches one of
       the ``--keep-archive`` globs. Otherwise it is *stripped*.
   Outcomes per archive: untouched / whole-strip / mixed (extract-strip-
   replace via ``ar``).
4. For each loose ``.o`` not already covered by a kept archive, look it
   up in the ninja index. If its source matches a ``--keep-prefix``, it
   stays. Otherwise it is stripped with ``strip --strip-debug``.
5. Archives the ninja index does not know about (e.g. Qt's
   ``libQtWebEngineCoreSandbox.a`` built by CMake, not GN) are kept by
   default; ``--strip-orphans`` flips that.
"""

from __future__ import annotations

import argparse
import fnmatch
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_BUILD_DIR = (
    SCRIPT_DIR / "src" / "build-UCRT64" / "src" / "core" / "RelWithDebInfo" / "AMD64"
)
DEFAULT_STRIP = "strip.exe"
DEFAULT_AR = "ar.exe"
DEFAULT_KEEP_PREFIXES: tuple[str, ...] = ()
DEFAULT_KEEP_ARCHIVES: tuple[str, ...] = ()
AR_BATCH = 64

CHROMIUM_TAIL_RE = re.compile(r"/src/3rdparty/chromium/(.+)$")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
RSP_TOKEN_RE = re.compile(r'"([^"]+)"|(\S+)')
COMPILE_RULES = {"cxx", "cc", "asm", "objc", "objcxx"}
OBJECT_SUFFIXES = (".o", ".obj")


def normalize_source(token: str) -> str | None:
    s = token.replace("\\", "/")
    m = CHROMIUM_TAIL_RE.search(s)
    return m.group(1) if m else None


def build_ninja_indexes(
    build_dir: Path,
) -> tuple[dict[str, str], dict[str, list[str]]]:
    obj_to_input: dict[str, str] = {}
    archive_to_objs: dict[str, list[str]] = {}
    for nf in build_dir.rglob("*.ninja"):
        try:
            text = nf.read_text(errors="replace", encoding="utf-8")
        except OSError:
            continue
        for line in text.splitlines():
            if not line.startswith("build "):
                continue
            body = line[len("build "):]
            head_sep = body.find(":")
            if head_sep < 0:
                continue
            outputs = body[:head_sep].strip().split()
            rest = body[head_sep + 1:].strip()
            toks = rest.split()
            if not toks:
                continue
            rule = toks[0]
            inputs: list[str] = []
            for t in toks[1:]:
                if t in ("|", "||"):
                    break
                inputs.append(t)
            if rule in COMPILE_RULES and outputs and inputs:
                obj_to_input[outputs[0].replace("\\", "/")] = inputs[0]
            elif rule == "alink" and outputs:
                archive = outputs[0].replace("\\", "/")
                if archive.endswith(".a"):
                    archive_to_objs[archive] = [
                        i.replace("\\", "/") for i in inputs
                    ]
    return obj_to_input, archive_to_objs


def parse_rsp(rsp_path: Path) -> list[Path]:
    """Return every path token the rsp references (quoted or unquoted)."""
    try:
        text = rsp_path.read_text(errors="replace", encoding="utf-8")
    except OSError:
        return []
    paths: list[Path] = []
    for m in RSP_TOKEN_RE.finditer(text):
        tok = m.group(1) or m.group(2)
        if not tok:
            continue
        # Drop @file references and pure linker flags.
        if tok.startswith(("@", "-", "/")) and not tok.endswith(OBJECT_SUFFIXES + (".a",)):
            continue
        paths.append(Path(tok))
    return paths


def parse_jumbo_includes(path: Path) -> set[str]:
    sources: set[str] = set()
    try:
        text = path.read_text(errors="replace", encoding="utf-8")
    except OSError:
        return sources
    for line in text.splitlines():
        m = INCLUDE_RE.match(line)
        if not m:
            continue
        rel = normalize_source(m.group(1))
        if rel:
            sources.add(rel)
    return sources


def resolve_obj_sources(
    obj_path: str,
    obj_to_input: dict[str, str],
    build_dir: Path,
    jumbo_cache: dict[Path, set[str]],
) -> set[str]:
    raw = obj_to_input.get(obj_path)
    if raw is None:
        return set()
    rel = normalize_source(raw)
    if rel is not None:
        return {rel}
    try:
        full = (build_dir / raw).resolve()
    except OSError:
        return set()
    if full in jumbo_cache:
        return jumbo_cache[full]
    if full.is_file():
        inc = parse_jumbo_includes(full)
        jumbo_cache[full] = inc
        return inc
    jumbo_cache[full] = set()
    return set()


def matches_keep(sources: set[str], prefixes: tuple[str, ...]) -> bool:
    return any(s.startswith(p) for s in sources for p in prefixes)


def strip_whole(target: Path, strip_bin: str) -> tuple[int, str | None]:
    try:
        before = target.stat().st_size
    except OSError as e:
        return 0, f"stat: {e}"
    try:
        subprocess.run(
            [strip_bin, "--strip-debug", str(target)],
            check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
        )
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        return 0, str(e)
    return before - target.stat().st_size, None


def strip_per_object(
    archive: Path,
    strip_objs: list[str],
    strip_bin: str,
    ar_bin: str,
) -> tuple[int, str | None]:
    """Extract members of *archive* whose basenames are in *strip_objs*,
    strip them, replace them in the archive (preserving original member
    order)."""
    strip_basenames = {Path(o).name for o in strip_objs}
    if not strip_basenames:
        return 0, None
    try:
        before = archive.stat().st_size
    except OSError as e:
        return 0, f"stat: {e}"
    with tempfile.TemporaryDirectory(prefix="qtwe-strip-") as tmp:
        tmp_path = Path(tmp)
        bnames = sorted(strip_basenames)
        for i in range(0, len(bnames), AR_BATCH):
            batch = bnames[i:i + AR_BATCH]
            r = subprocess.run(
                [ar_bin, "x", str(archive)] + batch,
                cwd=tmp_path, capture_output=True, text=True,
            )
            if r.returncode != 0:
                for bn in batch:
                    subprocess.run(
                        [ar_bin, "x", str(archive), bn],
                        cwd=tmp_path, capture_output=True,
                    )
        extracted = sorted(p for p in tmp_path.iterdir() if p.is_file())
        if not extracted:
            return 0, None
        for f in extracted:
            subprocess.run(
                [strip_bin, "--strip-debug", str(f)],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            )
        for i in range(0, len(extracted), AR_BATCH):
            batch = [str(f) for f in extracted[i:i + AR_BATCH]]
            r = subprocess.run(
                [ar_bin, "r", str(archive)] + batch,
                capture_output=True, text=True,
            )
            if r.returncode != 0:
                return 0, f"ar r failed: {r.stderr.strip()}"
        subprocess.run(
            [ar_bin, "s", str(archive)],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
    return before - archive.stat().st_size, None


def universe_from_rsps(
    rsp_paths: list[Path],
) -> tuple[list[Path], list[Path]]:
    """Return ([archives], [objects]) referenced by the given rsp files,
    de-duplicated and existing on disk."""
    seen_a: dict[Path, None] = {}
    seen_o: dict[Path, None] = {}
    for rsp in rsp_paths:
        if not rsp.is_file():
            print(f"  warning: rsp file not found: {rsp}", file=sys.stderr)
            continue
        for p in parse_rsp(rsp):
            try:
                resolved = p.resolve()
            except OSError:
                continue
            if not resolved.is_file():
                continue
            suffix = resolved.suffix.lower()
            if suffix == ".a":
                seen_a.setdefault(resolved, None)
            elif suffix in OBJECT_SUFFIXES:
                seen_o.setdefault(resolved, None)
    return sorted(seen_a), sorted(seen_o)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR,
                    help=f"build dir to scan (default: {DEFAULT_BUILD_DIR})")
    ap.add_argument("--rsp-file", action="append", default=None, type=Path,
                    metavar="PATH",
                    help="linker response file (.rsp) to drive the strip "
                         "from. Repeatable. If omitted, falls back to "
                         "rglob('*.a') under --build-dir.")
    ap.add_argument("--keep-prefix", action="append", default=None,
                    metavar="PATH",
                    help="chromium-relative source-path prefix; any object "
                         "whose source is under this prefix keeps its debug "
                         "info. Repeatable.")
    ap.add_argument("--keep-archive", action="append", default=None,
                    metavar="GLOB",
                    help="glob pattern against the archive path relative to "
                         "--build-dir; matching archives are kept whole. "
                         "Repeatable.")
    ap.add_argument("--strip", default=DEFAULT_STRIP,
                    help=f"strip executable (default: {DEFAULT_STRIP})")
    ap.add_argument("--ar", default=DEFAULT_AR,
                    help=f"ar executable (default: {DEFAULT_AR})")
    ap.add_argument("--dry-run", action="store_true",
                    help="report decisions without modifying anything")
    ap.add_argument("--strip-orphans", action="store_true",
                    help="also strip archives the ninja index does not know "
                         "about (default: keep them)")
    ap.add_argument("--quiet", action="store_true",
                    help="suppress per-file lines; only print the summary")
    args = ap.parse_args()

    if not args.build_dir.is_dir():
        sys.exit(f"build dir not found: {args.build_dir}")
    keep_prefixes = tuple(p if p.endswith("/") else p + "/"
                          for p in (args.keep_prefix or DEFAULT_KEEP_PREFIXES))
    keep_archive_patterns = tuple(args.keep_archive or DEFAULT_KEEP_ARCHIVES)
    if keep_prefixes:
        print(f"Keeping debug for sources under: {', '.join(keep_prefixes)}")
    if keep_archive_patterns:
        print(f"Keeping debug whole for archives matching: "
              f"{', '.join(keep_archive_patterns)}")
    if not keep_prefixes and not keep_archive_patterns:
        print("No --keep-prefix or --keep-archive supplied; every chromium "
              "input will be stripped.")

    print("Indexing ninja files...")
    obj_to_input, archive_to_objs = build_ninja_indexes(args.build_dir)
    print(f"Indexed {len(obj_to_input)} objects across "
          f"{len(archive_to_objs)} archive build rules")

    if args.rsp_file:
        archives, loose_objs = universe_from_rsps(args.rsp_file)
        print(f"Loaded {len(archives)} archives and {len(loose_objs)} loose "
              f"objects from {len(args.rsp_file)} rsp file(s)")
    else:
        archives = sorted(args.build_dir.rglob("*.a"))
        loose_objs = []
        print(f"Found {len(archives)} archive files via rglob")

    jumbo_cache: dict[Path, set[str]] = {}
    untouched = 0
    whole_strip = 0
    mixed = 0
    orphans_kept = 0
    orphans_stripped = 0
    obj_kept = 0
    obj_stripped = 0
    obj_orphans = 0
    failures: list[tuple[Path, str]] = []
    saved_bytes = 0

    # ---- archives ----
    for a in archives:
        try:
            rel = a.relative_to(args.build_dir).as_posix()
        except ValueError:
            rel = str(a).replace("\\", "/")
        if any(fnmatch.fnmatch(rel, p) for p in keep_archive_patterns):
            untouched += 1
            if not args.quiet:
                print(f"  KEEP  {rel}  (--keep-archive match)")
            continue

        objs = archive_to_objs.get(rel)
        if objs is None:
            if args.strip_orphans:
                if args.dry_run:
                    if not args.quiet:
                        print(f"  ORPHAN-STRIP {rel}")
                else:
                    bytes_, err = strip_whole(a, args.strip)
                    if err:
                        failures.append((a, err))
                    else:
                        saved_bytes += bytes_
                        if not args.quiet:
                            print(f"  orphan-stripped {rel}: "
                                  f"-{bytes_/(1024*1024):.0f} MB")
                orphans_stripped += 1
            else:
                if not args.quiet:
                    print(f"  ORPHAN-KEEP  {rel}")
                orphans_kept += 1
            continue

        keep_objs: list[str] = []
        strip_objs: list[str] = []
        for o in objs:
            srcs = resolve_obj_sources(o, obj_to_input,
                                       args.build_dir, jumbo_cache)
            if matches_keep(srcs, keep_prefixes):
                keep_objs.append(o)
            else:
                strip_objs.append(o)

        if not strip_objs:
            untouched += 1
            if not args.quiet:
                print(f"  KEEP  {rel}  ({len(keep_objs)} objs)")
            continue
        if not keep_objs:
            whole_strip += 1
            if args.dry_run:
                if not args.quiet:
                    print(f"  STRIP {rel}  ({len(strip_objs)} objs)")
            else:
                bytes_, err = strip_whole(a, args.strip)
                if err:
                    failures.append((a, err))
                else:
                    saved_bytes += bytes_
                    if not args.quiet:
                        print(f"  stripped {rel}: "
                              f"-{bytes_/(1024*1024):.0f} MB")
            continue
        mixed += 1
        if args.dry_run:
            if not args.quiet:
                print(f"  MIXED {rel}  (keep {len(keep_objs)}, "
                      f"strip {len(strip_objs)})")
            continue
        bytes_, err = strip_per_object(a, strip_objs, args.strip, args.ar)
        if err:
            failures.append((a, err))
        else:
            saved_bytes += bytes_
            if not args.quiet:
                print(f"  per-obj  {rel}  (keep {len(keep_objs)}, "
                      f"strip {len(strip_objs)}, "
                      f"-{bytes_/(1024*1024):.1f} MB)")

    # ---- loose objects (passed directly to the linker) ----
    for o in loose_objs:
        try:
            rel = o.relative_to(args.build_dir).as_posix()
        except ValueError:
            rel = str(o).replace("\\", "/")
        srcs = resolve_obj_sources(rel, obj_to_input,
                                   args.build_dir, jumbo_cache)
        if not srcs:
            # Object the chromium ninja index doesn't know about - typically
            # a Qt-side .obj built by CMake.
            if not args.strip_orphans:
                obj_orphans += 1
                if not args.quiet:
                    print(f"  OBJ-KEEP   {rel}  (orphan)")
                continue
            # Fall through to strip path; count as obj_stripped below.
        elif matches_keep(srcs, keep_prefixes):
            obj_kept += 1
            if not args.quiet:
                print(f"  OBJ-KEEP   {rel}")
            continue
        obj_stripped += 1
        if args.dry_run:
            if not args.quiet:
                print(f"  OBJ-STRIP  {rel}")
            continue
        bytes_, err = strip_whole(o, args.strip)
        if err:
            failures.append((o, err))
        else:
            saved_bytes += bytes_
            if not args.quiet and bytes_ >= 1024 * 1024:
                print(f"  obj-stripped {rel}: "
                      f"-{bytes_/(1024*1024):.0f} MB")

    print()
    print(f"Archives: untouched={untouched}, whole-strip={whole_strip}, "
          f"mixed={mixed}, "
          f"orphans-kept={orphans_kept}, orphans-stripped={orphans_stripped}")
    print(f"Objects:  kept={obj_kept}, stripped={obj_stripped}, "
          f"orphans-kept={obj_orphans}")
    if not args.dry_run:
        print(f"Total saved: {saved_bytes/(1024*1024):.1f} MB")
    if failures:
        print(f"Failures ({len(failures)}):")
        for f, msg in failures:
            print(f"  {f}: {msg}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())