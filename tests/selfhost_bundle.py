#!/usr/bin/env python3
"""Deterministic Stage0-only SE module bundler.

Transforms the SE-written compiler module graph into one SE compilation unit,
with private functions namespaced to avoid collisions. This is a bootstrap
tool, NOT a substitute for the SE compiler and not used to compile Stage2/3.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent
ROOT_SE = ROOT / "selfhost"
ORDER = ["lexer", "parser", "symbol_table", "typechecker",
         "codegen", "compile", "compiler"]
FUNCTION = re.compile(r"^make\s+([A-Za-z_][A-Za-z0-9_]*)\b", re.MULTILINE)
IMPORT = re.compile(r"^use\s+([A-Za-z_][A-Za-z0-9_]*)\s*$", re.MULTILINE)
IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
MODULES = set(ORDER)


def replace_names(line: str, mapping: dict[str, str]) -> str:
    """Rename identifiers only in code, never in strings or comments."""
    out: list[str] = []
    i = 0
    while i < len(line):
        if line[i] == "#":
            out.append(line[i:])
            break
        if line[i] in {"'", '"'}:
            quote = line[i]
            start = i
            i += 1
            while i < len(line):
                if line[i] == "\\":
                    i += 2
                    continue
                if line[i] == quote:
                    i += 1
                    break
                i += 1
            out.append(line[start:i])
            continue
        match = IDENT.match(line, i)
        if match:
            name = match.group()
            out.append(mapping.get(name, name))
            i = match.end()
            continue
        out.append(line[i])
        i += 1
    return "".join(out)


def build() -> str:
    sources = {name: (ROOT_SE / (name + ".se")).read_text(encoding="utf-8")
               for name in ORDER}
    definitions: dict[str, list[str]] = {
        name: FUNCTION.findall(source) for name, source in sources.items()
    }
    if any(len(values) != len(set(values)) for values in definitions.values()):
        raise ValueError("Duplicate function definition within an SE module")
    exports: dict[str, dict[str, str]] = {
        module: {name: module + "__" + name for name in names}
        for module, names in definitions.items()
    }

    builtins: set[str] = set()
    sections: list[str] = []
    for module in ORDER:
        source = sources[module]
        imports = IMPORT.findall(source)
        for imported in imports:
            if imported not in MODULES:
                builtins.add(imported)
        mapping: dict[str, str] = {}
        # Imported exported names are available as bare identifiers.
        for imported in imports:
            mapping.update(exports.get(imported, {}))
        # Local module definitions override imported names (e.g. _node).
        mapping.update(exports[module])
        lines: list[str] = []
        for line in source.splitlines():
            if IMPORT.fullmatch(line):
                continue
            lines.append(replace_names(line, mapping))
        sections.append(
            "# Stage0-bundled module: " + module + "\n" + "\n".join(lines).strip() + "\n"
        )
    return ("# Deterministic SE compiler bootstrap bundle.\n"
            "# Original source: selfhost/{lexer,parser,symbol_table,typechecker,codegen,compile,compiler}.se\n"
            + "\n".join("use " + name for name in sorted(builtins)) + "\n\n"
            + "\n".join(sections) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    dest = args.output.resolve()
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(build(), encoding="utf-8")
    print(f"Bundled SE compiler: {dest}")


if __name__ == "__main__":
    main()
