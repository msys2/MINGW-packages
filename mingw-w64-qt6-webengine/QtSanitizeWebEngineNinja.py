#!/usr/bin/env python3
"""Drop build edges for artifacts already supplied by qt6-webengine-thirdparty.

The set to skip is read from the manifests that package ships, not from a
pattern matched against paths.  A pattern has to be kept in sync by hand at
every site that uses it, and anything it fails to cover is silently built
twice -- once by the producer as a dependency, then again here.
"""
import pathlib
import re
import sys

import QtWebEngineNinjaGraph as ninjagraph


def read_manifest_entries(path, rebase_on_obj=False):
    """Read ordered build-dir-relative paths from an rsp/manifest.

    Entries may be absolute in either msys (/ucrt64/...) or mixed
    (E:/msys64/...) form depending on who wrote them, so rebasing keys off the
    /obj/ component rather than stripping a prefix we would have to guess.
    """
    entries = []
    if not path.is_file():
        return entries
    for line in path.read_text(encoding="utf-8").splitlines():
        token = line.strip().strip('"')
        if not token:
            continue
        token = token.replace("\\", "/")
        if rebase_on_obj:
            head, separator, tail = token.partition("/obj/")
            if not separator:
                continue
            token = "obj/" + tail
        entries.append(token.removeprefix("./"))
    return entries


def read_manifest(path, rebase_on_obj=False):
    return set(read_manifest_entries(path, rebase_on_obj))


def read_prebuilt_outputs(path):
    """Read the copied ``gen/`` output manifest.

    Only paths beneath ``gen/`` are accepted.  The package creates this list
    while copying the producer's generated tree, so it describes actual files
    rather than a suffix or target-name guess.
    """
    entries = read_manifest(path)
    invalid = sorted(entry for entry in entries if not entry.startswith("gen/"))
    if invalid:
        sys.exit("error: {} has non-generated path {!r}".format(path, invalid[0]))
    return entries


def unpack_object_archive(archive, build_dir, object_paths):
    """Materialise the producer's loose source_set objects into the build dir.

    GN source_sets do not go through an archive: their objects are named
    individually on the final link line. The producer archive is transport only.
    LLVM ar flattens its member names, so pair members with the ordered manifest
    and require each basename to match before restoring the full ``obj/...``
    destination. This also preserves colliding basenames from different paths.

    Linking the packed archive instead would be smaller but not equivalent --
    archive members are pulled in only to satisfy an undefined symbol, so
    translation units that matter solely for their static initialisers (feature
    and trace-category registration, mojo type converters) would be dropped.
    That failure is silent at link time and only shows up at runtime.
    """
    members = 0
    with archive.open("rb") as handle:
        if handle.read(8) != b"!<arch>\n":
            sys.exit("error: {} is not an ar archive".format(archive))

        string_table = b""
        while True:
            header = handle.read(60)
            if len(header) < 60:
                break

            raw_name = header[0:16].decode("ascii", "replace").strip()
            size = int(header[48:58].decode("ascii").strip())
            payload = handle.read(size)
            if size % 2:
                handle.read(1)

            if raw_name == "//":
                # GNU long-name table: every path here exceeds ar's 15-char
                # inline limit, so nearly all real members resolve through it.
                string_table = payload
                continue
            if raw_name == "/" or raw_name == "/SYM64/":
                continue  # symbol index

            if raw_name.startswith("/") and raw_name[1:].isdigit():
                offset = int(raw_name[1:])
                if offset >= len(string_table):
                    sys.exit(
                        "error: {} has an invalid long-name offset {}".format(
                            archive, offset
                        )
                    )
                # GNU ar terminates long names with "/\n", while COFF archives
                # produced by LLVM use NUL.  Accept both encodings.
                ends = [
                    end
                    for marker in (b"\n", b"\0")
                    if (end := string_table.find(marker, offset)) != -1
                ]
                end = min(ends) if ends else len(string_table)
                name = string_table[offset:end].decode("ascii").rstrip("/")
            elif raw_name.startswith("#1/") and raw_name[3:].isdigit():
                # BSD extended names are stored at the start of the member and
                # included in its declared size.
                name_size = int(raw_name[3:])
                if name_size > len(payload):
                    sys.exit(
                        "error: {} has a truncated BSD extended name".format(archive)
                    )
                name = payload[:name_size].rstrip(b"\0").decode("ascii")
                payload = payload[name_size:]
            else:
                name = raw_name.rstrip("/")

            if not name:
                continue
            if "\0" in name:
                sys.exit("error: {} has a NUL in member name {!r}".format(archive, name))

            member_path = pathlib.PurePosixPath(name)
            if (
                member_path.is_absolute()
                or ".." in member_path.parts
                or re.match(r"^[A-Za-z]:", name)
            ):
                sys.exit(
                    "error: {} has unsafe member name {!r}".format(archive, name)
                )
            if members >= len(object_paths):
                sys.exit(
                    "error: {} has more object members than the manifest".format(
                        archive
                    )
                )

            object_path = pathlib.PurePosixPath(object_paths[members])
            if member_path.name != object_path.name:
                sys.exit(
                    "error: {} member {} is {!r}, expected manifest object {!r}".format(
                        archive, members, name, object_paths[members]
                    )
                )
            destination = build_dir / object_path
            # Re-writing 3 GB on every configure is pure wall-clock; the objects
            # are immutable build output, so a size match means identical.
            if destination.is_file() and destination.stat().st_size == len(payload):
                members += 1
                continue

            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(payload)
            members += 1

    if members != len(object_paths):
        sys.exit(
            "error: {} has {} object members but the manifest lists {}".format(
                archive, members, len(object_paths)
            )
        )
    missing = [path for path in object_paths if not (build_dir / path).is_file()]
    if missing:
        sys.exit(
            "error: {} did not restore manifest object {!r}".format(
                archive, missing[0]
            )
        )
    return members


build_dir = pathlib.Path(sys.argv[1])
private_dir = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else None
prebuilt_manifest = pathlib.Path(sys.argv[3]) if len(sys.argv) > 3 else None

supplied_archives = set()
supplied_object_entries = []
supplied_objects = set()
if private_dir is not None:
    supplied_archives = read_manifest(
        private_dir / "qtwebengine-thirdparty-archives.rsp",
        rebase_on_obj=True,
    )
    supplied_object_entries = read_manifest_entries(
        private_dir / "qtwebengine-thirdparty-objects.rsp"
    )
    supplied_objects = set(supplied_object_entries)
prebuilt_outputs = (read_prebuilt_outputs(prebuilt_manifest)
                    if prebuilt_manifest is not None else set())

if not supplied_archives:
    sys.exit(
        "error: no archive manifest found under {}; refusing to run without a "
        "skip set, as that would silently rebuild everything the producer "
        "already shipped".format(private_dir)
    )


unpacked_objects = 0
if supplied_objects:
    object_archive = private_dir / "libQt6WebEngineThirdPartyObjects.a"
    if not object_archive.is_file():
        sys.exit(
            "error: {} lists {} objects but {} is missing; the link names those "
            "objects by path, so they have to exist".format(
                private_dir, len(supplied_objects), object_archive.name
            )
        )
    unpacked_objects = unpack_object_archive(
        object_archive, build_dir, supplied_object_entries
    )


def is_supplied_archive(path: str) -> bool:
    path = path.replace("\\", "/").removeprefix("./")
    return (
        path in supplied_archives
        or pathlib.PurePosixPath(path).name == "libQtWebEngineCoreSandbox.a"
    )


def is_supplied_object(path: str) -> bool:
    return path.replace("\\", "/").removeprefix("./") in supplied_objects


changed_files = 0
changed_edges = 0
removed_inputs = 0
phonied_compiles = 0

for ninja_file in (build_dir / "obj").rglob("*.ninja"):
    original = ninja_file.read_text(encoding="utf-8")
    output_lines = []
    file_changed = False

    for line in original.splitlines(keepends=True):
        ending = "\r\n" if line.endswith("\r\n") else "\n" if line.endswith("\n") else ""
        content = line.removesuffix(ending)
        # A supplied object is already on disk, unpacked above, but ninja still
        # refuses to load an edge whose source file is gone -- and the sources
        # for wholly-supplied subtrees are deliberately deleted.  Dropping the
        # rule is what makes recompiling them impossible rather than merely
        # redundant.
        compiled = re.match(r"^build (\S+): (?:cxx|cc|objcxx|objc|asm|rc) ", content)
        if compiled is not None and is_supplied_object(compiled.group(1)):
            output_lines.append(f"build {compiled.group(1)}: phony{ending}")
            file_changed = True
            phonied_compiles += 1
            continue

        match = re.match(r"^build (.+): alink(?: (.*))?$", content)
        if match is None:
            output_lines.append(line)
            continue

        outputs = match.group(1)
        inputs = (match.group(2) or "").split()
        output_paths = [token for token in outputs.split() if token != "|"]
        make_phony = any(is_supplied_archive(path) for path in output_paths)

        if make_phony:
            replacement = f"build {outputs}: phony{ending}"
            removed_inputs += len(inputs)
        else:
            filtered_inputs = [
                token for token in inputs if not is_supplied_object(token)
            ]
            removed_inputs += len(inputs) - len(filtered_inputs)
            replacement = (
                f"build {outputs}: alink"
                + (f" {' '.join(filtered_inputs)}" if filtered_inputs else "")
                + ending
            )

        if replacement != line:
            file_changed = True
            changed_edges += 1
        output_lines.append(replacement)

    if file_changed:
        ninja_file.write_text("".join(output_lines), encoding="utf-8", newline="")
        changed_files += 1

# Generated assets copied from the producer are valid build outputs but must
# remain leaves: their original actions may need a host-only generator that the
# consumer intentionally does not ship (for example ts-proto).  Use the graph
# rather than a list of known actions, so every copied output follows the same
# contract.  An edge is replaced only when *all* of its outputs were packaged;
# otherwise its remaining outputs still require the original action.
prebuilt_edges = partial_prebuilt_edges = 0
prebuilt_files = 0
if prebuilt_outputs:
    edges, _, _ = ninjagraph.load(build_dir)
    replacements = []
    for edge in edges:
        outputs = edge.outputs + edge.implicit_outputs
        supplied = set(outputs) & prebuilt_outputs
        if not supplied:
            continue
        if supplied != set(outputs):
            partial_prebuilt_edges += 1
            continue
        replacements.append((
            edge,
            "build {}: phony".format(" ".join(
                ninjagraph.escape(output) for output in outputs
            )),
        ))
        prebuilt_edges += 1
    prebuilt_files = ninjagraph.replace_edges(replacements)

print(
    f"supplied_archives={len(supplied_archives)} "
    f"supplied_objects={len(supplied_objects)} "
    f"prebuilt_outputs={len(prebuilt_outputs)} "
    f"prebuilt_edges={prebuilt_edges} "
    f"partial_prebuilt_edges={partial_prebuilt_edges} "
    f"unpacked_objects={unpacked_objects} "
    f"sanitized_files={changed_files + prebuilt_files} "
    f"sanitized_edges={changed_edges} "
    f"phonied_compiles={phonied_compiles} "
    f"removed_inputs={removed_inputs}"
)

if supplied_objects and phonied_compiles == 0:
    sys.exit(
        "error: {} objects are supplied but no compile edge was dropped; the "
        "build would recompile every one of them".format(len(supplied_objects))
    )
