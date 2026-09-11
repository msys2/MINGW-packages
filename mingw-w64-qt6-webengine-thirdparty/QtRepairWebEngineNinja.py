#!/usr/bin/env python3
"""Restore build ordering that the generated ninja graph fails to express.

Two defects make a build from an empty tree fail partway through, at a point
that depends on how ninja happened to schedule things:

  1. Mojom's generate_type_mappings.py reads sibling ``*__type_mappings`` files
     named by ``--dependency`` on its command line.  Some of those are not
     listed as ninja inputs, so nothing orders them first and the action dies
     with ``OSError: Missing dependencies: ...``.

  2. Compile edges carry no ordering to the generated headers their translation
     units include -- most visibly ``*.pb.h``.  Only 33 of the 1133 generated
     protobuf headers are reachable from the link at all, so a compile can be
     scheduled before the header it needs exists.

Both come from the same place: the GN patch that keeps this package from
building third-party code the -thirdparty package already supplies drops
dependencies by substring-matching their path against ``third_party/``,
``skia/`` and ``protobuf``.  That is the right filter for a *link* input -- an
object we do not build must not be named on the link line -- but ordering edges
to generated headers match it too and are lost with them.  Hence
``gen/skia/public/mojom/mojom__type_mappings`` going missing while the
neighbouring ``gen/mojo/public/mojom/base/base__type_mappings`` survives.

Rather than re-derive which dependency was legitimate inside GN, this rebuilds
the ordering from evidence in the build directory itself: the ``--dependency``
arguments say what an action reads, and ``#include`` says what a translation
unit needs.  Both are ground truth, and both stay correct as the patch set
moves.  Nothing here alters what is linked.
"""
import collections
import os
import pathlib
import re
import sys

import QtWebEngineNinjaGraph as ninjagraph

AGGREGATE = "qtwebengine_generated_prerequisites"
DRIVE_RE = re.compile(r"^[A-Za-z]:[\\/]")
INCLUDE_RE = re.compile(rb'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]',
                        re.MULTILINE)
SOURCE_SUFFIXES = (".cc", ".cpp", ".cxx", ".c", ".h", ".hpp", ".inc", ".hxx")
LINK_ROOTS = ("QtWebEngineCore", "QtWebEngineCore.stamp", "convert_dict",
              "convert_dict.stamp", "sandboxLibrary")


def in_build_dir(build_dir, token):
    """Turn a manifest path into one the running interpreter can stat.

    Prebuilt tools are named with a Windows drive letter while everything else
    is relative to the build directory.  Joining the two is wrong under msys
    python, which reads ``E:/...`` as a relative name and silently reports the
    tool missing -- so the two cases are separated here rather than left to
    os.path.join to guess.
    """
    return token if DRIVE_RE.match(token) else os.path.join(build_dir, token)


def reachable(producer, roots):
    seen = {root for root in roots if root in producer}
    stack = list(seen)
    while stack:
        edge = producer.get(stack.pop())
        if edge is None:
            continue
        for dep in edge.all_inputs():
            if dep not in seen:
                seen.add(dep)
                stack.append(dep)
    return seen


def closure(producer, seeds):
    return reachable(producer, seeds)


def find_unbuildable(build_dir, producer, targets):
    """Map each target that cannot be built to the input that is missing.

    Qt's release tarball drops Chromium's test and policy-template inputs while
    keeping the BUILD.gn rules that consume them, so a slice of the graph
    describes work that can never run.  Ninja treats one such input as fatal
    for the *whole invocation* -- it refuses to produce a plan at all, so this
    has to be excluded up front rather than tolerated at build time.  These
    targets are only ever reached through headers that the preprocessor skips.
    """
    WHITE, GREY, BLACK = 0, 1, 2
    color, cause = {}, {}
    for target in targets:
        if color.get(target, WHITE) == BLACK:
            continue
        stack = [target]
        while stack:
            node = stack[-1]
            state = color.get(node, WHITE)
            if state == BLACK:
                stack.pop()
                continue
            edge = producer.get(node)
            if edge is None:
                cause[node] = (None if os.path.isfile(
                    in_build_dir(build_dir, node)) else node)
                color[node] = BLACK
                stack.pop()
                continue
            if state == WHITE:
                color[node] = GREY
                for dep in edge.all_inputs():
                    if color.get(dep, WHITE) == WHITE:
                        stack.append(dep)
                continue
            missing = None
            for dep in edge.all_inputs():
                if cause.get(dep):
                    missing = cause[dep]
                    break
            cause[node] = missing
            color[node] = BLACK
            stack.pop()
    return {target: cause[target] for target in targets if cause.get(target)}


class IncludeResolver:
    """Map ``#include`` spellings onto ninja outputs under ``gen/``.

    The generated headers are mostly not on disk yet when this runs, so a
    spelling is resolved by asking the graph whether ``gen/<spelling>`` is
    something ninja knows how to produce.  A file that is already present wins,
    which matches what the preprocessor will do and, more importantly, leaves
    the generated headers build() copies in from the -thirdparty package alone:
    those already work, and ordering compiles behind a fresh regeneration of
    them would be a change this repair has no reason to make.
    """

    def __init__(self, build_dir, gen_outputs, search_dirs):
        self.build_dir = build_dir
        self.gen_outputs = gen_outputs
        self.search_dirs = search_dirs
        self.exists = {}
        self.scanned = {}
        self.by_spelling = {}

    def on_disk(self, relative):
        hit = self.exists.get(relative)
        if hit is None:
            hit = os.path.isfile(in_build_dir(self.build_dir, relative))
            self.exists[relative] = hit
        return hit

    def _search(self, spelling):
        for directory in self.search_dirs:
            candidate = os.path.normpath(
                os.path.join(directory, spelling)).replace("\\", "/")
            if self.on_disk(candidate):
                return None, candidate
            if candidate in self.gen_outputs:
                return candidate, None
        return None, None

    def resolve(self, spelling, origin_dir):
        """Return (generated_output, disk_path); at most one is set.

        The search path is tried first and its answer memoised on the spelling
        alone: nearly every include in this tree is written relative to the
        source root, so caching here is what keeps the scan from turning into
        millions of stat calls.  Sibling-relative includes fall through to the
        origin directory.
        """
        hit = self.by_spelling.get(spelling)
        if hit is None:
            hit = self._search(spelling)
            self.by_spelling[spelling] = hit
        if hit != (None, None) or not origin_dir:
            return hit
        candidate = os.path.normpath(
            os.path.join(origin_dir, spelling)).replace("\\", "/")
        if self.on_disk(candidate):
            return None, candidate
        if candidate in self.gen_outputs:
            return candidate, None
        return None, None

    def scan(self, start_files):
        """Collect every generated header reachable through #include."""
        generated = set()
        queue = collections.deque(start_files)
        seen = set()
        while queue:
            relative = queue.popleft()
            if relative in seen:
                continue
            seen.add(relative)
            cached = self.scanned.get(relative)
            if cached is None:
                try:
                    with open(in_build_dir(self.build_dir, relative),
                              "rb") as handle:
                        blob = handle.read()
                except OSError:
                    self.scanned[relative] = ((), ())
                    continue
                origin = os.path.dirname(relative)
                hits, follow = [], []
                for match in INCLUDE_RE.findall(blob):
                    spelling = match.decode("utf-8", "replace")
                    if not spelling.endswith(SOURCE_SUFFIXES):
                        continue
                    made, disk = self.resolve(spelling, origin)
                    if made is not None:
                        hits.append(made)
                    elif disk is not None:
                        follow.append(disk)
                cached = (tuple(hits), tuple(follow))
                self.scanned[relative] = cached
            generated.update(cached[0])
            queue.extend(cached[1])
        return generated


def main():
    build_dir = pathlib.Path(sys.argv[1])
    edges, rules, manifests = ninjagraph.load(build_dir)

    producer = {}
    for edge in edges:
        for out in edge.outputs + edge.implicit_outputs:
            producer.setdefault(out, edge)

    # A resumed build runs build() from the top.  The generated CMake helper may
    # also have stripped compile order-only inputs since the previous repair, so
    # an existing aggregate is reused while any missing references are restored.
    aggregate_edge = producer.get(AGGREGATE)

    additions = []

    # ---- ordering the --dependency arguments of mojom actions ---------------
    repaired_actions = repaired_refs = 0
    for edge in edges:
        command = rules.get(edge.rule, {}).get("command", "")
        if "--dependency" not in command:
            continue
        tokens = command.split()
        declared = set(edge.all_inputs())
        wanted = []
        for index, token in enumerate(tokens[:-1]):
            if token != "--dependency":
                continue
            path = ninjagraph.unescape(tokens[index + 1])
            if path not in declared and path in producer and path not in wanted:
                wanted.append(path)
        if not wanted:
            continue
        prefix = "" if edge.order_only else " ||"
        additions.append(
            (edge, prefix + "".join(" " + ninjagraph.escape(p) for p in wanted))
        )
        repaired_actions += 1
        repaired_refs += len(wanted)

    # ---- ordering compiles behind the headers they include -----------------
    gen_outputs = {out for out in producer if out.startswith("gen/")}

    search_dirs = []
    seen_dir = set()
    compiles = [edge for edge in edges if edge.is_compile()]
    for edge in compiles:
        for token in ninjagraph.split_tokens(
            (edge.scope or {}).get("include_dirs", "")
        ):
            if not token.startswith("-I"):
                continue
            directory = ninjagraph.unescape(token[2:])
            if directory not in seen_dir:
                seen_dir.add(directory)
                search_dirs.append(directory)

    from_link = reachable(producer, LINK_ROOTS)
    linked = [edge for edge in compiles if edge.outputs[0] in from_link]
    if not linked:
        sys.exit("error: no compile edge is reachable from {}; the roots this "
                 "scopes the scan to no longer exist".format(list(LINK_ROOTS)))

    resolver = None
    unbuildable = {}
    if aggregate_edge is not None:
        # Reuse the cold-tree result.  Generated headers may now be on disk and
        # the resolver intentionally prefers those files, so rescanning a warm
        # tree would shrink the aggregate and lose the original cycle analysis.
        required = set(aggregate_edge.all_inputs())
    else:
        # A jumbo translation unit is itself generated and so is not on disk yet;
        # its real sources are the inputs of the edge that will concatenate it.
        start_files = set()
        for edge in linked:
            for source in edge.inputs:
                if os.path.isfile(in_build_dir(build_dir, source)):
                    start_files.add(source)
                elif source in producer:
                    start_files.update(
                        nested for nested in producer[source].inputs
                        if nested.endswith(SOURCE_SUFFIXES)
                        and os.path.isfile(in_build_dir(build_dir, nested))
                    )

        resolver = IncludeResolver(build_dir, gen_outputs, search_dirs)
        required = resolver.scan(start_files)
        if not resolver.scanned:
            sys.exit("error: resolved no translation unit to scan from {} compile "
                     "edges; the graph no longer has the shape this expects"
                     .format(len(linked)))

        unbuildable = find_unbuildable(build_dir, producer, required)
        required -= set(unbuildable)
        for target, missing in sorted(unbuildable.items()):
            print("excluded {}: needs absent {}".format(target, missing))

    # Anything needed to produce those headers must not be made to wait on
    # them.  In practice the generators are prebuilt and this set holds no
    # compile output at all, but a cycle here would make ninja refuse the
    # whole graph, so it is checked rather than assumed.
    generator_inputs = closure(producer, required)
    skipped_cyclic = ordered_compiles = 0
    for edge in compiles:
        if edge.outputs[0] in generator_inputs:
            skipped_cyclic += 1
            continue
        if AGGREGATE in edge.order_only:
            continue
        prefix = "" if edge.order_only else " ||"
        additions.append((edge, prefix + " " + AGGREGATE))
        ordered_compiles += 1

    touched = ninjagraph.append_to_edges(additions)

    if aggregate_edge is None:
        with (build_dir / "build.ninja").open(
                "a", encoding="utf-8", newline="") as fh:
            fh.write("\n# added by QtRepairWebEngineNinja.py\n")
            fh.write("build {}: phony".format(AGGREGATE))
            for path in sorted(required):
                fh.write(" $\n    " + ninjagraph.escape(path))
            fh.write("\n")

    print(
        f"manifests={len(manifests)} edges={len(edges)} "
        f"repaired_actions={repaired_actions} repaired_refs={repaired_refs} "
        f"compiles={len(compiles)} linked_compiles={len(linked)} "
        f"ordered_compiles={ordered_compiles} "
        f"scanned_sources={len(resolver.scanned) if resolver else 0} "
        f"generated_prerequisites={len(required)} "
        f"excluded_unbuildable={len(unbuildable)} "
        f"skipped_cyclic={skipped_cyclic} "
        f"aggregate_created={int(aggregate_edge is None)} "
        f"rewritten_manifests={touched}"
    )


if __name__ == "__main__":
    main()
