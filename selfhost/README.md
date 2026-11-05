# SE Self-Hosted Compiler — Experimental Bootstrap

This compiler is **implemented in SE** and translates a supported core SE subset to C++20. The existing production SE compiler is still written in C++ and remains authoritative; the experimental bootstrap has passed independent Stage2/3/4 self-compilation for its supported subset. Neither full SE syntax nor all 117+ standard-library modules are supported. Unsupported features fail explicitly.

本目錄是用 **SE 語言本身**編寫的編譯器前端與 C++20 後端。保留原本正式 C++ 編譯器不動。此實驗性編譯器已通過 Stage2/3/4 獨立自我編譯驗證，但只涵蓋本文列出的 SE 子集；它仍不是支援完整 SE 語言與所有 extensions 的編譯器。

## Architecture / 架構

```
source.se
  ↓
lexer.se               token stream [kind, lexeme, line, column]
  ↓
parser.se              Pratt expression parser and structured JSON AST
  ↓
symbol_table.se        symbol registration and scope snapshots
  ↓
typechecker.se         static checks: types, arity, undeclared names, annotated returns and collection keys
  ↓
codegen.se             C++20 translation
  + se_bootstrap_runtime.hpp (small supporting runtime)
  ↓
output.cpp → c++ -std=c++20 → native executable
```

The SE-written public driver is `compiler.se`, which uses `compile.se` to orchestrate the stages. The AST uses JSON-encoded `Text` nodes in a `List[Text]` to respect SE's homogeneous List constraints. Nested statement blocks are JSON-serialized and Base64-packed by `ast_codec.se`, avoiding repeated escaping as nested blocks are embedded.

## Run / 執行

From the **repository root**, after building the experimental branch's `se` executable (or using a compatible SE installation):

```sh
se check selfhost/compiler.se
SE_SELFHOST_SOURCE=selfhost/cases/language_core.se \
SE_SELFHOST_OUTPUT=selfhost/generated.cpp \
se run selfhost/compiler.se
c++ -std=c++20 -O0 -Iselfhost selfhost/generated.cpp -o selfhost/program
./selfhost/program
```

Set `SE_SELFHOST_SOURCE` and `SE_SELFHOST_OUTPUT` to another source/target. Defaults are `selfhost/sample.se` and `selfhost/generated_sample.cpp`. The C++ output includes `se_bootstrap_runtime.hpp`; keep that header accessible via the `-Iselfhost` include option.

## Supported compiler features / 已支援

- **Lexer**: ASCII identifiers, numeric and string literals, comments, indent/dedent, operators, typed function tokens and source locations.
- **Parser**: expression precedence, lists/maps/sets, indexing and calls, `ask` / `try` expressions, assignments, `if/elif/else if/else`, `match/case/else`, `while/repeat/for`, `try/else error` recovery, `for key value in map`, generic and ordinary functions, and basic `type` declarations with instance field initialization.
- **Static checks**: symbol existence, assignments, numeric and boolean operands, homogeneous lists, indexing, function arity, declared parameter types, annotated return types, selected block scopes, `ask` types, explicit failures, conversion aliases (`bool`/`boolean` included), and Error `message`/`kind` fields.
- **Zero-argument user functions**: bare identifiers call zero-parameter functions automatically, while ordinary values and callbacks requiring arguments stay unchanged, matching the production evaluator.
- **C++ backend**: Int, Num, Bool, Text, homogeneous Lists, insertion-ordered Maps and Sets, common arithmetic/logic/conditions, inclusive range loops, recursion, indexed collection updates, functions, simple module helpers, input/output and exceptions.
- **Nested functions**: lexical nested `make` declarations compile to capturing local C++ lambdas; fully annotated local functions use `std::function` for recursion. Escaping closure lifetimes and nested generic functions are not supported.
- **Generic user types**: `type Box[T]`, multi-parameter user types such as `Pair[A,B]`, specialized constructors (`Box[Int]`) and nested annotated fields are lowered to C++20 class templates with checked argument counts and field types.
- **Generic functions**: simple generic functions such as `make identity[T] item:T -> T` compile to C++ templates. Multiple generic parameters and nested generic type inference still require additional testing.
- **Canonical bare-call arithmetic**: arguments absorb arithmetic (`twice a + b` means `twice(a+b)`), while `func a - 1 + func a - 2` splits into two calls when the next operand starts another call. Both have native/interpreter differential tests.
- **Block layout**: blank lines and comment-only lines between a block header and its first indented statement are accepted.
- **Local source modules**: `use helpers` recursively resolves `helpers.se` or `helpers.s` relative to the importer; duplicate imports are deduplicated and circular imports rejected. Imported declarations are flattened, so full module namespaces/encapsulation are not implemented.
- **C++ keyword safety**: SE identifiers such as `class`, `template` and `namespace` are escaped consistently during C++ generation.
- **Typed forward functions**: C++20 prototypes are emitted before definitions for functions whose parameter and return types are explicit, enabling forward calls and mutual recursion. Calls requiring return-type deduction are not yet supported forward.
- **Library subset**: math operations and constants, `text` split/trim/contains/repeat/join, `collections` sort/reverse/take/drop/slice/filter/map/reduce/sort_with, `json` String/List[String] wire encoding, `os` getenv/platform/cwd, `random` numeric generators, filesystem `path` operations, `time` clock conversion/formatting, `file` managed handles plus read/write/copy/move/directory operations, basic `test` assertions, and core conversions, help text, and Unicode `char`. The native bootstrap runtime is separate from the production VM; this is **not** full standard-library compatibility.

## Tests / 測試

```sh
se run selfhost/parser_test.se
se run selfhost/parser_extended_test.se
se run selfhost/typechecker_test.se
se run selfhost/typechecker_extended_test.se
se run selfhost/codegen_test.se
python3 tests/selfhost_differential.py $(command -v se)
# Strict test of the *actual compiler* self-compiling, not a sample program:
python3 tests/selfhost_stage2.py $(command -v se)
```

The test compiles generated C++20 and runs the actual executable. Where syntax is supported by both implementations, it compares output with the production SE interpreter; for typed multi-input (not accepted by the production interpreter), it verifies explicit expected output instead. It also asserts that invalid programs are rejected. Positive cases include Map/Set insertion, lookup and mutation, typed multi-input, recursion, typed mutual recursion, indexed assignments, conversions and Unicode characters. It also covers arithmetic, for/while/repeat, string operations and diagnostic failures.

The strict verifier bootstraps once with production SE, then runs generated native compilers that link only the standalone bootstrap runtime. This proves self-compilation for the bundled module graph and supported subset; it does not prove complete SE or extension coverage.

## Independent runtime / 獨立執行階段

The generated C++20 sample programs compile against ONLY
`selfhost/se_bootstrap_runtime.hpp` and the host C++ standard library,
not the production SE VM (`libs_core`). The independent runtime includes Base64 encoding/decoding for compact nested AST blocks. Run
`c++ -std=c++20 -O0 -Iselfhost tests/selfhost_runtime.cpp -o /tmp/se-runtime-test && /tmp/se-runtime-test`
to exercise independent text, collection, Unicode-character, Map/Set and error paths.
**This proves standalone subset-program execution only**, not an independent,
self-compiled SE compiler.

## Strict self-hosting test / 嚴格自編譯驗證

`tests/selfhost_stage2.py` is an intentionally strict verifier. It must:
1. Bootstrap the SE-written compiler with the production CLI **once**.
2. Compile the compiler's own source from the generated native executable.
3. Compile subsequent generations without invoking the production interpreter.
4. Compare Stage 2, Stage 3 and Stage 4 generated C++ SHA-256 hashes.

A failure is a **failure**, never counted as a passing self-host result.
Local source imports now work in the tested subset, but full module namespace
semantics, many standard-library APIs, some nested-list mutation rules and
advanced language constructs are missing. A successful Stage2/3/4 proof shows that this supported bundled compiler can compile itself deterministically; it does not imply complete language or extension compatibility.

## Native bootstrap build performance / 原生編譯控制

The production CLI now accepts the strictly validated variable
`SE_NATIVE_OPT=0` for a source-native debug build; default `-O2` behavior is
unchanged. Example:

```sh
SE_NATIVE_OPT=0 SE_NATIVE_CORE_LIB="$(pwd)/build/libs_core.a" build/se build selfhost/compiler.se
```

The optional `SE_NATIVE_CORE_LIB` fast path reuses an archive built by CMake, e.g. `cmake --build build --target se`; it still links the **production** runtime and is not proof of independent execution. The strict verifier uses this mode to avoid recompiling production runtime sources or optimizing the large Stage0 serialized AST. Even if the Stage0 native
executable builds and runs, the independent Stage2/3/4 proof still has to pass.

## What is NOT finished / 尚待完成

- Full generic user types and all inference combinations, closures and nested functions, managed async/await, native ABI modules, SE Web, complete recovery diagnostics, and all language forms. Basic `type`, generic functions, and value-based `match` are implemented as documented above.
- Complete standard library and platform/ABI compatibility; the production VM supports more modules than this bootstrap, and imported module availability is not equivalent to full API coverage.
- Comprehensive error recovery, Unicode/UTF-8 identifiers, source maps and source-accurate diagnostics.
- Broader self-host coverage: the strict Stage2/Stage3/Stage4 test passes for the currently bundled subset. Source bundling plus a production Stage0 is not proof by itself; changes must preserve native Stage1/2/3 self-compilation without linking or invoking the production VM after bootstrap.
- Cross-platform SE-only bootstrap on Linux/macOS/Windows is **not** verified. The fast differential suite currently runs on Linux only.

### Principle

Unsupported syntax must error instead of silently generating incorrect code. Compare both behavior and diagnostics against the production SE implementation before expanding the accepted language.

## Recursive types and annotated fields

`type_model.se` ports recursive type compatibility, generic binding and return-type substitution from the production checker. List/Map/Set C++ types are emitted recursively, including nested collection types. Repeated generic bindings are checked for conflicts. User-type fields support annotations, defaults, required initialization and checked field assignment. Generic user-type declarations themselves remain unfinished.

## Math, statistics and regular expressions

The bootstrap backend now maps the production math module's operational API to the independent runtime: constants pi/e/tau/inf, unary/binary functions, variadic hypot/min/max/gcd/lcm, interpolation, factorial/combinations/permutations, and population statistics. `statistics` provides mean/median and sample/population variance and deviation. `regex` and `re` provide match/search/replace/split using the same C++ ECMAScript regex implementation as production. Math combination overflow is explicitly rejected. Static module API checking and complete help output remain unfinished.
