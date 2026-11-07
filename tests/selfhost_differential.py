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
NATIVE = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None


def run(*argv: str, input_text: str = "", env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(x) for x in argv],
        input=input_text,
        text=True,
        capture_output=True,
        cwd=ROOT,
        env=env,
        timeout=180 if str(argv[0]) == CXX else 45,
        check=False,
    )


def verify_positive(case: str, data: str, tmp: Path, *, expected_output: str | None = None) -> None:
    src = CASE_DIR / case
    cpp = tmp / (case + ".cpp")
    exe = tmp / (case + (".exe" if os.name == "nt" else ""))
    env = dict(os.environ, SE_SELFHOST_SOURCE=str(src), SE_SELFHOST_OUTPUT=str(cpp))
    produced = run(*([str(NATIVE)] if NATIVE else [str(SE), "run", "selfhost/compiler.se"]), env=env)
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



def verify_native_load_failure(case: str, tmp: Path) -> None:
    src = CASE_DIR / case
    cpp = tmp / (case + ".cpp")
    exe = tmp / (case + (".exe" if os.name == "nt" else ""))
    env = dict(os.environ, SE_SELFHOST_SOURCE=str(src), SE_SELFHOST_OUTPUT=str(cpp))
    produced = run(*([str(NATIVE)] if NATIVE else [str(SE), "run", "selfhost/compiler.se"]), env=env)
    assert produced.returncode == 0, (case, "self-host compiler", produced.stderr)
    compiled = run(CXX, "-std=c++20", "-O0", "-g0", "-I", str(ROOT / "selfhost"), str(cpp), "-o", str(exe))
    assert compiled.returncode == 0, (case, "g++", compiled.stderr)
    actual = run(str(exe))
    assert actual.returncode != 0, (case, "missing library should fail during module import")
    assert "Could not load native library" in actual.stderr, (case, repr(actual.stderr))
    assert "unreachable" not in actual.stdout, (case, repr(actual.stdout))
    print(f"PASS eager native load failure: {case}")


def verify_negative(case: str, part: str, tmp: Path) -> None:
    src = CASE_DIR / case
    cpp = tmp / (case + ".cpp")
    env = dict(os.environ, SE_SELFHOST_SOURCE=str(src), SE_SELFHOST_OUTPUT=str(cpp))
    proc = run(*([str(NATIVE)] if NATIVE else [str(SE), "run", "selfhost/compiler.se"]), env=env)
    assert proc.returncode != 0, (case, "expected failure")
    assert part in proc.stderr, (case, repr(proc.stderr), part)
    assert not cpp.exists(), (case, "compiled invalid source")
    print(f"PASS rejected: {case}")


def main() -> None:
    for filename in ("lexer.se", "parser.se", "symbol_table.se", "typechecker.se", "codegen.se", "compile.se", "compiler.se"):
        result = run(str(SE), "check", str(ROOT / "selfhost" / filename))
        assert result.returncode == 0, (filename, result.stdout, result.stderr)
    for filename in ("parser_test.se", "parser_extended_test.se", "parser_match_test.se", "typechecker_test.se", "typechecker_extended_test.se", "codegen_test.se"):
        result = run(str(SE), "run", str(ROOT / "selfhost" / filename))
        assert result.returncode == 0, (filename, result.stdout, result.stderr)
        print(f"PASS unit: {filename}")
    with tempfile.TemporaryDirectory(prefix="se-selfhost-") as folder:
        tmp = Path(folder)
        verify_positive("language_sized_arrays.se", "", tmp, expected_output="5\n0\n7\n2\n\nSE\nfalse\n2\n3\n42\n3\n")
        verify_positive("language_object_aliases.se", "", tmp)
        verify_positive("language_collection_aliases.se", "", tmp)
        verify_positive("language_series.se", "", tmp)
        verify_positive("language_linear_algebra.se", "", tmp)
        verify_positive("language_expansion_numeric.se", "", tmp)
        verify_positive("language_regex_uuid.se", "", tmp)
        verify_positive("language_operator_module.se", "", tmp)
        verify_positive("language_iter_combinatorics.se", "", tmp, expected_output="2\n7\n6\n8\n6\n1\n3\n2\n3\n0\n3\n")
        verify_positive("language_enum_module.se", "", tmp, expected_output="3\nrunning\n2\ntrue\nfalse\n")
        verify_positive("language_datetime_iter.se", "", tmp, expected_output="1970-01-01T00:00:00Z\n1970-01-01\n15\n4\n1\n3\n3\n3\n2\n2\n6\n")
        verify_positive("language_itertools_glob.se", "", tmp, expected_output="5\n2\n2\n2\n2\n3\n2\n3\n6\n2\ntrue\ntrue\n4869\n7\ntrue\n")
        verify_positive("language_extension_utilities.se", "", tmp, expected_output="SE\n42\ntrue\n42\nlight\nsession=ok; Path=/; HttpOnly; SameSite=Lax\nOrigin\nGET, POST\n&lt;&amp;&gt;\nHi &lt;SE&gt;!\ntext/css; charset=utf-8\nInt\ntrue\na%20b\n5345\n")
        verify_positive("language_codec_extensions.se", "", tmp, expected_output="a%20b%2F%2B\na b/+\na%20b=c%2Bd&percent=%25\nc+d\n%\n\n5345\nSE\ntrue\ntrue\n")
        verify_positive("language_web_utilities.se", "", tmp, expected_output="1\ntwo\nsession=ok; Path=/; HttpOnly; SameSite=Lax\nOrigin\nGET, POST\n&lt;&amp;&gt;\nHello &lt;SE&gt;!\n")
        verify_positive("language_tilemap_extensions.se", "", tmp, expected_output="2\n2\nB\nC\n")
        verify_positive("language_csv.se", "", tmp, expected_output="CSV roundtrip passed\n")
        verify_positive("language_dynamic_any.se", "", tmp, expected_output="Int\nText\nBool\ndynamic Any ok\n")
        verify_positive("language_evaluation_order.se", "one\ntwo\nthree\nfour\n", tmp, expected_output="first: second: [one, two]\nthird: fourth: threefour\n")
        verify_positive("language_copy.se", "", tmp, expected_output="9\n9\n7\n1\n")
        verify_positive("language_generic_callbacks.se", "", tmp, expected_output="[1, 2, 3]\n[2, 4, 6]\n42\n12\n7\n22\n3.75\n")
        verify_positive("language_native_abi.se", "", tmp, expected_output="42\n16\ntrue\nnative text\n88\n11\n89\n11\nNativeError\n42\n0\n")
        verify_native_load_failure("language_native_eager_failure.se", tmp)
        verify_positive("language_core.se", "", tmp)
        verify_positive("language_text.se", "", tmp)
        verify_positive("language_text_replace.se", "", tmp)
        verify_positive("language_math_extended.se", "", tmp)
        verify_positive("language_statistics_regex.se", "", tmp)
        verify_positive("language_conversions.se", "", tmp)
        verify_positive("language_time.se", "", tmp)
        verify_positive("language_file_module.se", "", tmp)
        verify_positive("language_generic_tests.se", "", tmp)
        verify_positive("language_generic_index.se", "", tmp)
        verify_positive("language_nested_generic.se", "", tmp)
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
        verify_positive("language_fibonacci_canonical.se", "", tmp)
        verify_positive("language_bare_arithmetic.se", "", tmp)
        verify_positive("language_std_basic.se", "", tmp)
        verify_positive("language_user_type.se", "", tmp)
        verify_positive("language_typed_fields.se", "", tmp)
        verify_positive("language_generic_user_types.se", "", tmp)
        verify_positive("language_nested_functions.se", "", tmp)
        verify_positive("language_escaping_closures.se", "", tmp)
        verify_positive("language_nested_generic_functions.se", "", tmp)
        verify_positive("language_tasks_functions.se", "", tmp)
        verify_positive("language_global_bindings.se", "", tmp)
        verify_positive("language_recursive_collection_mutation.se", "", tmp)
        verify_positive("language_collections_functional.se", "", tmp)
        verify_positive("language_remove_collections.se", "", tmp)
        verify_positive("language_remove_map.se", "", tmp)
        verify_positive("language_cpp_keywords.se", "", tmp)
        verify_positive("language_source_imports.se", "", tmp)
        verify_positive("language_module_namespaces.se", "", tmp)
        verify_positive("language_s_imports.se", "", tmp)
        verify_positive("language_package_imports.se", "", tmp)
        verify_positive("language_duration.se", "", tmp, expected_output="duration ok\n")
        verify_positive("language_elif.se", "", tmp, expected_output="middle\n")
        # Production runtime auto-invokes nullary functions, but its static
        # checker currently rejects their arithmetic use as Function + Int.
        # Exercise the actual generated C++ against a fixed output oracle.
        verify_positive("language_zero_arg_functions.se", "", tmp, expected_output="42\n43\nready\ntrue\n84\n")
        for case, message in (
            ("negative_sized_array_type.se", "Sized arrays support Int, Num, Bool or Text"),
            ("negative_sized_array_dimension.se", "Sized array dimensions must be Int."),
            ("negative_unknown_annotation.se", "Unknown type annotation"),
            ("negative_private_import.se", "Undefined function"),
            ("negative_tilemap_arity.se", "tilemap.at expects 3 arguments"),
            ("negative_tilemap_type.se", "tilemap.size expects List[Text]"),
            ("negative_codec_type.se", "encoding.hex expects Text"),
            ("negative_codec_arity.se", "url.decode expects one argument"),
            ("negative_codec_member.se", "Unknown url function"),
            ("negative_duplicate_export.se", "provided by more than one module"),
            ("negative_generic_rebind.se", "was already inferred as"),
            ("negative_required_field.se", "Missing required field"),
            ("negative_generic_wrong_arity.se", "needs 1 type arguments"),
            ("negative_generic_wrong_field.se", "Initializer field type mismatch"),
            ("negative_try_missing_else.se", "try block needs else"),
            ("negative_field_default.se", "Field default type mismatch"),
            ("negative_wrong_type.se", "Arithmetic requires numbers"),
            ("negative_undefined.se", "Undefined identifier"),
            ("negative_wrong_arity.se", "Wrong arity"),
            ("negative_zero_arg_wrong_arity.se", "Wrong arity"),
            ("negative_wrong_return.se", "Function return type"),
            ("negative_hetero_list.se", "List element type mismatch"),
            ("negative_bad_ask.se", "ask supports Text, Int or Num"),
            ("negative_index_type.se", "Collection assignment expects Int"),
            ("negative_map_key.se", "Map keys must be Text"),
            ("negative_set_value.se", "Set values must have the same type"),
            ("negative_missing_return.se", "is missing a return"),
            ("negative_match_type.se", "Match case type mismatch"),
            ("negative_for_pairs_not_map.se", "Two-variable for expects a Map"),
            ("negative_unknown_type_field.se", "Unknown initializer field"),
            ("negative_cyclic_import.se", "Cyclic SE module import"),
            ("negative_missing_source_import.se", "SE module not found"),
        ):
            verify_negative(case, message, tmp)
    print("SE selfhost differential tests passed")


if __name__ == "__main__":
    main()
