#!/usr/bin/env python3
"""Minimal ninja manifest reader: enough to walk and amend the build graph.

Only what this repair needs is modelled -- no variable expansion, no rule
inheritance beyond ``command``.  Edges remember which physical lines they came
from so a caller can append to them without re-serialising a path it would
otherwise have to re-escape correctly.
"""
import pathlib


def unescape(token):
    out = []
    index = 0
    while index < len(token):
        char = token[index]
        if char == "$" and index + 1 < len(token) and token[index + 1] in ":$ ":
            out.append(token[index + 1])
            index += 2
            continue
        out.append(char)
        index += 1
    return "".join(out)


def escape(path):
    return path.replace("$", "$$").replace(":", "$:").replace(" ", "$ ")


def split_tokens(text):
    """Split on whitespace that is not escaped with '$'."""
    tokens = []
    current = []
    index = 0
    while index < len(text):
        char = text[index]
        if char == "$" and index + 1 < len(text):
            current.append(char)
            current.append(text[index + 1])
            index += 2
            continue
        if char in " \t":
            if current:
                tokens.append("".join(current))
                current = []
            index += 1
            continue
        current.append(char)
        index += 1
    if current:
        tokens.append("".join(current))
    return tokens


class Edge:
    __slots__ = ("outputs", "implicit_outputs", "rule", "inputs", "implicit",
                 "order_only", "path", "first_line", "last_line", "scope")

    def __init__(self):
        self.outputs = []
        self.implicit_outputs = []
        self.rule = ""
        self.inputs = []
        self.implicit = []
        self.order_only = []
        self.path = None
        self.first_line = 0
        self.last_line = 0
        self.scope = None

    def all_inputs(self):
        return self.inputs + self.implicit + self.order_only

    def is_compile(self):
        return self.rule in ("cxx", "cc", "objcxx", "objc", "asm")


def _join_continuations(path):
    """Yield (first_line, last_line, text) with trailing-'$' joins resolved."""
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        pending = ""
        first = 0
        for lineno, raw in enumerate(handle, 1):
            line = raw.rstrip("\n").rstrip("\r")
            if pending:
                joined = pending + line.lstrip()
            else:
                first = lineno
                joined = line
            trailing = len(joined) - len(joined.rstrip("$"))
            if trailing % 2 == 1:
                pending = joined[:-1]
                continue
            pending = ""
            yield first, lineno, joined
        if pending:
            yield first, lineno, pending


def _split_at_colon(text):
    index = 0
    while index < len(text):
        if text[index] == "$":
            index += 2
            continue
        if text[index] == ":":
            return text[:index], text[index + 1:]
        index += 1
    return None, None


def parse_manifest(path, edges, rules, root):
    """Parse one manifest, appending to `edges`/`rules`; return its includes."""
    included = []
    scope = {}
    edge = None
    rule = None
    for first, last, line in _join_continuations(path):
        if not line.strip():
            edge = rule = None
            continue
        if line[0] in " \t":
            key, sep, value = line.strip().partition("=")
            if not sep:
                continue
            if rule is not None:
                rules[rule][key.strip()] = value.strip()
            continue
        edge = rule = None
        if line.startswith("rule "):
            rule = line[5:].strip()
            rules.setdefault(rule, {})
            continue
        if line.startswith(("subninja ", "include ")):
            included.append(root / unescape(line.split(None, 1)[1].strip()))
            continue
        if not line.startswith("build "):
            key, sep, value = line.partition("=")
            if sep and " " not in key.strip():
                scope[key.strip()] = value.strip()
            continue

        out_part, in_part = _split_at_colon(line[6:])
        if out_part is None:
            continue
        in_tokens = split_tokens(in_part)
        if not in_tokens:
            continue

        edge = Edge()
        edge.path = path
        edge.first_line = first
        edge.last_line = last
        edge.scope = scope

        section = edge.outputs
        for token in split_tokens(out_part):
            if token == "|":
                section = edge.implicit_outputs
                continue
            section.append(unescape(token))

        edge.rule = in_tokens[0]
        section = edge.inputs
        for token in in_tokens[1:]:
            if token == "|":
                section = edge.implicit
            elif token == "||":
                section = edge.order_only
            else:
                section.append(unescape(token))
        edges.append(edge)
    return included


def load(build_dir):
    """Parse build.ninja and everything it pulls in."""
    build_dir = pathlib.Path(build_dir)
    edges, rules = [], {}
    queue = [build_dir / "build.ninja"]
    seen = set()
    while queue:
        path = queue.pop()
        if str(path) in seen or not path.is_file():
            continue
        seen.add(str(path))
        queue.extend(parse_manifest(path, edges, rules, build_dir))
    return edges, rules, seen


def append_to_edges(additions):
    """Append raw text to the last physical line of each edge.

    Keyed by manifest so every file is rewritten once.  Appending rather than
    re-emitting keeps us from having to reproduce ninja's escaping of the parts
    we are not touching.
    """
    by_file = {}
    for edge, text in additions:
        by_file.setdefault(edge.path, []).append((edge.last_line, text))
    for path, items in by_file.items():
        # readlines() splits exactly where the parser's line iteration did;
        # str.splitlines() would also break on form feeds and so could number
        # the lines differently and append to the wrong edge.
        with open(path, "r", encoding="utf-8", errors="surrogateescape") as fh:
            lines = fh.readlines()
        for lineno, text in items:
            line = lines[lineno - 1]
            ending = "\n" if line.endswith("\n") else ""
            lines[lineno - 1] = line[: len(line) - len(ending)] + text + ending
        with open(path, "w", encoding="utf-8", errors="surrogateescape",
                  newline="") as fh:
            fh.writelines(lines)
    return len(by_file)
