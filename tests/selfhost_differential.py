#!/usr/bin/env python3
"""Differential checks: SE-written compiler vs production SE interpreter.

This test does not use a fake compiler response: it invokes SE source through
SE CLI, compiles the generated C++20, runs the resulting executable, and
compares output against the production interpreter on the same SE source.
"""
from __future__ import annotations
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
CASE_DIR = ROOT / "selfhost" / "cases"
SE = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "se-0.7.6-linux-x64" / "bin" / "se"
CXX = os.environ.get("CXX", "c++")


def run(*argv: str, input_text: str = "", env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(x) for x in argv],
        input=input_text,
        text=True,
        capture_output=True,
        cwd=ROOT,
        env=env,
        timeout=45,
        check=False,
    )


def verify_positive(case: str, data: str, tmp: Path, *, expected_output: str | None = None) -> None:
    src = CASE_DIR / case
    cpp = tmp / (case + ".cpp")
    exe = tmp / (case + (".exe" if os.name == "nt" else ""))
    env = dict(os.environ, SE_SELFHOST_SOURCE=str(src), SE_SELFHOST_OUTPUT=str(cpp))
    produced = run(str(SE), "run", "selfhost/compiler.se", env=env)
    assert produced.returncode == 0, (case, "self-host compiler", produced.stderr)
    assert cpp.is_file(), f"Missing generated source for {case}"
    compiled = run(CXX, "-std=c++20", "-O0", "-g0", "-I", str(ROOT / "selfhost"), str(cpp), "-o", str(exe))
    assert compiled.returncode == 0, (case, "g++", compiled.stderr)
    actual = run(str(exe), input_text=data)
    if expected_output is not None:
        # Some valid bootstrap-subset syntax (typed multi-ask) has not yet
        # been implemented in the production interpreter. Verify the actual
        # generated executable against an explicit output oracle, not a fake
        # interpreter result.
        assert actual.returncode == 0, (case, actual.stderr)
        assert actual.stdout == expected_output, (case, repr(actual.stdout), repr(expected_output))
        print(f"PASS bootstrap execution: {case}")
        return
    expected = run(str(SE), "run", str(src), input_text=data)
    assert actual.returncode == expected.returncode, (case, actual.stderr, expected.stderr)
    native_stdout = re.sub(r"^Input \d+ space-separated values: ", "", expected.stdout)
    assert actual.stdout == native_stdout, (case, repr(actual.stdout), repr(native_stdout))
    print(f"PASS differential: {case}")


def verify_negative(case: str, part: str, tmp: Path) -> None:
    src = CASE_DIR / case
    cpp = tmp / (case + ".cpp")
    env = dict(os.environ, SE_SELFHOST_SOURCE=str(src), SE_SELFHOST_OUTPUT=str(cpp))
    proc = run(str(SE), "run", "selfhost/compiler.se", env=env)
    assert proc.returncode != 0, (case, "expected failure")
    assert part in proc.stderr, (case, repr(proc.stderr), part)
    assert not cpp.exists(), (case, "compiled invalid source")
    print(f"PASS rejected: {case}")


def main() -> None:
    for filename in ("lexer.se", "parser.se", "symbol_table.se", "typechecker.se", "codegen.se", "compile.se", "compiler.se"):
        result = run(str(SE), "check", str(ROOT / "selfhost" / filename))
        assert result.returncode == 0, (filename, result.stderr)
    for filename in ("parser_test.se", "parser_extended_test.se", "parser_match_test.se", "typechecker_test.se", "typechecker_extended_test.se", "codegen_test.se"):
        result = run(str(SE), "run", str(ROOT / "selfhost" / filename))
        assert result.returncode == 0, (filename, result.stderr)
        print(f"PASS unit: {filename}")
    with tempfile.TemporaryDirectory(prefix="se-selfhost-") as folder:
        tmp = Path(folder)
        verify_positive("language_core.se", "", tmp)
        verify_positive("language_text.se", "", tmp)
        verify_positive("language_input.se", "5 7 hello\n", tmp, expected_output="12\nhello\n")
        verify_positive("language_index_assignment.se", "", tmp)
        verify_positive("language_map_set.se", "", tmp)
        verify_positive("language_recursion.se", "", tmp)
        verify_positive("language_forward_functions.se", "", tmp)
        verify_positive("language_match.se", "", tmp)
        verify_positive("language_char_collections.se", "", tmp)
        verify_positive("language_json_ast.se", "", tmp)
        verify_positive("language_try_recovery.se", "", tmp)
        verify_positive("language_for_map_pairs.se", "", tmp)
        verify_positive("language_generic_functions.se", "", tmp)
        for case, message in (
            ("negative_wrong_type.se", "Arithmetic requires numbers"),
            ("negative_undefined.se", "Undefined identifier"),
            ("negative_wrong_arity.se", "Wrong arity"),
            ("negative_wrong_return.se", "Function return type"),
            ("negative_hetero_list.se", "List element type mismatch"),
            ("negative_bad_ask.se", "ask supports Text, Int or Num"),
            ("negative_index_type.se", "Collection assignment expects Int"),
            ("negative_map_key.se", "Map keys must be Text"),
            ("negative_set_value.se", "Set values must have the same type"),
            ("negative_missing_return.se", "is missing a return"),
            ("negative_match_type.se", "Match case type mismatch"),
            ("negative_for_pairs_not_map.se", "Two-variable for expects a Map"),
        ):
            verify_negative(case, message, tmp)
    print("SE selfhost differential tests passed")


if __name__ == "__main__":
    main()
