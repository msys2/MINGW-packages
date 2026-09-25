#!/usr/bin/env python3
"""Restore build ordering that the generated ninja graph fails to express.

Two defects make a build from an empty tree fail partway through, at a point
that depends on how ninja happened to schedule things:

  1. Actions read generated files that their edges omit: mojom modules and
     type mappings, DevTools protocols, JavaScript bundling/minification inputs,
     and TypeScript project dependencies. Nothing orders such a reader after the
     producer, so it dies with ``OSError: Missing dependencies: ...`` or
     ``FileNotFoundError``.

  2. Compile edges carry no ordering to generated inputs.  This is most
     visible for ``*.pb.h``, but generated jumbo translation units also include
     generated ``*.cc`` files.  A compile can be scheduled before either exists.

Both come from the same place: the GN patch that keeps this package from
building third-party code the -thirdparty package already supplies drops
dependencies by substring-matching their path against ``third_party/``,
``skia/`` and ``protobuf``.  That is the right filter for a *link* input -- an
object we do not build must not be named on the link line -- but ordering edges
to generated headers match it too and are lost with them.  Hence
``gen/skia/public/mojom/mojom__type_mappings`` going missing while the
neighbouring ``gen/mojo/public/mojom/base/base__type_mappings`` survives.

Rather than re-derive which dependency was legitimate inside GN, this rebuilds
the ordering from evidence in the build directory itself: generated paths in an
action recipe say what it reads, explicit input-directory and filename pairs
cover tools that split a path across arguments, and ``#include`` says what a
translation unit needs. Both stay correct as the patch set moves. Nothing here
alters what is linked.
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
SOURCE_SUFFIXES = (".cc", ".cpp", ".cxx", ".c", ".h", ".hpp", ".inc", ".hxx",
                   ".moc")
LINK_ROOTS = ("QtWebEngineCore", "QtWebEngineCore.stamp", "convert_dict",
              "convert_dict.stamp", "sandboxLibrary")
GENERATED_PATH_RE = re.compile(r"gen/[A-Za-z0-9_./$:+-]+")
INPUT_DIRECTORY_FLAGS = ("--input", "--in_folder")
INPUT_FILE_FLAGS = ("--in_files", "--js_module_in_files")


def normalize_command_path(edge, value):
    """Yield graph spellings for one command argument.

    GN action helpers commonly express generated inputs relative to the action's
    output directory (not the ninja working directory). Trying both spellings
    only accepts a candidate when it is an actual graph output, so source paths
    and tool arguments cannot become dependencies accidentally.
    """
    value = ninjagraph.unescape(value).rstrip(",;:)]}")
    if not value or DRIVE_RE.match(value):
        return
    yield os.path.normpath(value).replace("\\", "/")
    if edge.outputs:
        yield os.path.normpath(os.path.join(
            os.path.dirname(edge.outputs[0]), value)).replace("\\", "/")


def flag_values(tokens, flags, multiple=False):
    """Yield values belonging to selected command-line flags."""
    for index, token in enumerate(tokens):
        plain = ninjagraph.unescape(token)
        for flag in flags:
            if plain.startswith(flag + "="):
                yield plain[len(flag) + 1:]
                break
            if plain != flag:
                continue
            following = tokens[index + 1:]
            for value in following:
                value = ninjagraph.unescape(value)
                if value.startswith("--"):
                    break
                yield value
                if not multiple:
                    break
            break


def generated_cxx_inputs(edge, rule, gen_outputs):
    """Return generated C/C++ paths fed to a generated source action.

    Jumbo source lists are commonly emitted through rule-scoped
    ``rspfile_content`` rather than normal Ninja inputs. Edge-local values can
    override those defaults, so inspect both while accepting only actual graph
    outputs.
    """
    candidates = list(edge.inputs) + list(edge.implicit)
    for variables in (rule, edge.variables):
        for value in variables.values():
            candidates.extend(match.group(0) for match in
                              GENERATED_PATH_RE.finditer(value))
    return {
        path for path in candidates
        if path in gen_outputs and path.endswith((".cc", ".cpp", ".cxx", ".c"))
    }


def command_prerequisites(edge, command, producer):
    """Yield generated files an action recipe reads but does not declare.

    This treats the generated-output table as the authority. It recognizes
    literal paths (including ``key=path`` and paths embedded in configuration
    values), output-relative paths, and paths split between an explicit input
    directory and filename list. The final case covers bundlers without adding
    every output in a directory, some of which can be downstream of the action.
    """
    tokens = ninjagraph.split_tokens(command)
    candidates = []
    for token in tokens:
        plain = ninjagraph.unescape(token)
        values = [plain]
        if "=" in plain:
            values.append(plain.split("=", 1)[1])
        for value in values:
            # Bare GN target names occur in action commands too. They are graph
            # outputs, but they are not filesystem inputs and may alias this
            # action's own output, so only path-like values are considered.
            if "/" in value or "\\" in value:
                candidates.append(value)
            candidates.extend(match.group(0) for match in
                              GENERATED_PATH_RE.finditer(value))

    directories = list(flag_values(tokens, INPUT_DIRECTORY_FLAGS))
    filenames = list(flag_values(tokens, INPUT_FILE_FLAGS, multiple=True))
    candidates.extend(os.path.join(directory, filename)
                      for directory in directories for filename in filenames)

    wanted = set()
    for value in candidates:
        for path in normalize_command_path(edge, value):
            if path in producer:
                wanted.add(path)

    # A mojom generator reads its parser-produced module through a response
    # file. Ninja writes that response file immediately before launching the
    # action, so it is unavailable for inspection here. The output contract is
    # nevertheless explicit: every generated ``foo.mojom-*`` or
    # ``foo.test-mojom-*`` sibling uses the correspondingly named ``*-module``.
    # This is naming-based rather than tied to a Chromium target or a particular
    # response-file path.
    for output in edge.outputs + edge.implicit_outputs:
        markers = [marker for marker in
                   (output.find(".mojom"), output.find(".test-mojom"))
                   if marker >= 0]
        if markers:
            marker = min(markers)
            suffix = (".test-mojom" if output.startswith(".test-mojom", marker)
                      else ".mojom")
            module = output[:marker + len(suffix)] + "-module"
            if module in producer:
                wanted.add(module)
    return wanted


def reaches_any(producer, start, targets):
    """Return whether an existing dependency path reaches any target."""
    seen = set()
    stack = [start]
    while stack:
        node = stack.pop()
        if node in targets:
            return True
        if node in seen:
            continue
        seen.add(node)
        edge = producer.get(node)
        if edge is not None:
            stack.extend(edge.all_inputs())
    return False


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
        """Collect generated headers and source fragments reachable by include.

        Jumbo inputs are generated source files which textual-include generated
        ``.cc`` fragments.  Those fragments are compile prerequisites just as
        much as a generated header is, while following disk-resident sources is
        still needed to find nested generated headers.
        """
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

    # ---- ordering generated inputs that action recipes read -----------------
    repaired_actions = repaired_refs = skipped_action_cycles = 0
    for edge in edges:
        command = rules.get(edge.rule, {}).get("command", "")
        if not command:
            continue
        declared = set(edge.all_inputs())
        wanted = command_prerequisites(edge, command, producer) - declared
        own_outputs = set(edge.outputs + edge.implicit_outputs)
        wanted.difference_update(own_outputs)
        cyclic = {
            path for path in wanted
            if reaches_any(producer, path, own_outputs)
        }
        wanted.difference_update(cyclic)
        skipped_action_cycles += len(cyclic)
        if not wanted:
            continue
        ordered_wanted = sorted(wanted)
        prefix = "" if edge.order_only else " ||"
        additions.append(
            (edge, prefix + "".join(
                " " + ninjagraph.escape(path) for path in ordered_wanted))
        )
        # Later unbuildable and cycle analysis must inspect the graph that will be
        # written, not the stale graph loaded before these inferred inputs existed.
        edge.order_only.extend(ordered_wanted)
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
        # its real sources are the inputs of the edge that concatenates it.  Most
        # are on disk and can be scanned for headers.  Generated ``.cc`` inputs
        # are different: the jumbo file textual-includes them, so retain their
        # graph outputs as explicit prerequisites instead of hoping their action
        # wins Ninja's scheduling race.
        start_files = set()
        generated_sources = set()
        for edge in linked:
            for source in edge.inputs:
                if os.path.isfile(in_build_dir(build_dir, source)):
                    start_files.add(source)
                elif source in producer:
                    for nested in producer[source].inputs:
                        if not nested.endswith(SOURCE_SUFFIXES):
                            continue
                        if os.path.isfile(in_build_dir(build_dir, nested)):
                            start_files.add(nested)
                        elif nested.startswith("gen/") and nested in producer:
                            generated_sources.add(nested)

        resolver = IncludeResolver(build_dir, gen_outputs, search_dirs)
        # A jumbo merge records its source list in rule-scoped rspfile_content.
        # Ninja may also materialise the rsp file on a resumed build. GN mangles
        # the target and toolchain into the rule name, so match the stable rule
        # suffix rather than one fictional exact name.
        for edge in edges:
            rule = rules.get(edge.rule, {})
            if ("_jumbo_merge" not in edge.rule or
                    not any(output in from_link for output in
                            edge.outputs + edge.implicit_outputs)):
                continue
            jumbo_inputs = generated_cxx_inputs(edge, rule, gen_outputs)
            generated_sources.update(jumbo_inputs)
            # Jumbo inputs live only in rule-scoped rspfile_content, so Ninja does
            # not order the merge behind their generators. Record the same graph
            # relation used for aggregate discovery; this also lets the generic
            # unbuildable analysis reject a jumbo whose source cannot be produced.
            jumbo_inputs.difference_update(
                edge.outputs + edge.implicit_outputs + edge.all_inputs()
            )
            if jumbo_inputs:
                prefix = "" if edge.order_only else " ||"
                ordered_jumbo_inputs = sorted(jumbo_inputs)
                additions.append((
                    edge,
                    prefix + "".join(
                        " " + ninjagraph.escape(path)
                        for path in ordered_jumbo_inputs
                    ),
                ))
                edge.order_only.extend(ordered_jumbo_inputs)
            rspfile = edge.variables.get("rspfile", rule.get("rspfile"))
            if rspfile and os.path.isfile(in_build_dir(build_dir, rspfile)):
                with open(in_build_dir(build_dir, rspfile), encoding="utf-8",
                          errors="replace") as handle:
                    generated_sources.update(
                        match.group(0) for match in GENERATED_PATH_RE.finditer(
                            handle.read()
                        ) if match.group(0) in gen_outputs and match.group(0).endswith(
                            (".cc", ".cpp", ".cxx", ".c")
                        )
                    )
        required = resolver.scan(start_files) | generated_sources
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
        f"skipped_action_cycles={skipped_action_cycles} "
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
