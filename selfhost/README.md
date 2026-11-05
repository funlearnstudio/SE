# SE Self-Hosted Compiler — Experimental Bootstrap

This compiler is **implemented in SE** and translates a supported core SE subset to C++20. The existing production SE compiler is still written in C++ and remains authoritative; the experimental bootstrap has **not** compiled itself. Neither full SE syntax nor all 117+ standard-library modules are supported. Unsupported features fail explicitly.

本目錄是用 **SE 語言本身**編寫的編譯器前端與 C++20 後端。保留原本正式 C++ 編譯器不動。這是經端到端測試的 **SE 子集編譯器**，還不是完成自我編譯的完整 SE Compiler。

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

The SE-written public driver is `compiler.se`, which uses `compile.se` to orchestrate the stages. The AST uses JSON-encoded `Text` nodes in a `List[Text]` to respect SE's homogeneous List constraints.

## Run / 執行

From the **repository root**, after installing SE 0.7.6 or later:

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
- **Parser**: expression precedence including unary and binary operators, list literals, indexing, member access, calls, `ask` and `try` expressions; statements `say`, `give`, `fail`, assignments including `+=` etc., `if/else if/else`, `match/case/else`, `while`, `repeat`, `for`, typed and untyped `make`, `use`, and multi-variable typed `ask`.
- **Static checks**: symbol existence, assignments, numeric and boolean operands, homogeneous lists, indexing, function arity, declared parameter types, annotated return types, selected block scopes, `ask` types and explicit failures.
- **C++ backend**: Int, Num, Bool, Text, homogeneous Lists, insertion-ordered Maps and Sets, common arithmetic/logic/conditions, inclusive range loops, recursion, indexed collection updates, functions, simple module helpers, input/output and exceptions.
- **Typed forward functions**: C++20 prototypes are emitted before definitions for functions whose parameter and return types are explicit, enabling forward calls and mutual recursion. Calls requiring return-type deduction are not yet supported forward.
- **Library subset**: selected `math`, `text`, `collections` (`sort`, `reverse`, `take`, `drop`), `file`, and core conversion/read/write/append/Unicode-`char` operations only. This **does not imply** general library compatibility.

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

The test compiles generated C++20 and runs the actual executable. Where syntax is supported by both implementations, it compares output with the production SE interpreter; for typed multi-input (not accepted by the production interpreter), it verifies explicit expected output instead. It also asserts that invalid programs are rejected. Positive cases include Map/Set insertion, lookup and mutation, typed multi-input, recursion, typed mutual recursion and indexed assignments. It also covers arithmetic, for/while/repeat, string operations and diagnostic failures.

A separately bootstrapped native compiler program can be produced from `selfhost/compiler.se` **using the production C++ SE compiler**. That binary may still link the production `s_core` VM, so it **does not prove true self-hosting or independent compiler runtime**.

## Independent runtime / 獨立執行階段

The generated C++20 sample programs compile against ONLY
`selfhost/se_bootstrap_runtime.hpp` and the host C++ standard library,
not the production SE VM (`libs_core`). Run
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
The current compiler lacks custom-module imports, several standard-library
functions, mutable nested-list semantics and full language constructs; the
Stage 2/3 check is therefore not expected to pass yet.

## What is NOT finished / 尚待完成

- Complete parser and checker for `type`, generic Map/Set shapes, generics, closures, `match`, async, native modules, SE Web and all language forms.
- Complete standard library and platform/ABI compatibility.
- Comprehensive error recovery, Unicode/UTF-8 identifiers, source maps and source-accurate diagnostics.
- Self-compile and reproducibility: compiler written in SE compiling **its own** sources, then stage2-stage3 binary equivalence.
- Cross-platform SE-only bootstrap verified on Linux/macOS/Windows. Differential tests currently verified in Linux; GitHub Actions needed for other platforms.

### Principle

Unsupported syntax must error instead of silently generating incorrect code. Compare both behavior and diagnostics against the production SE implementation before expanding the accepted language.
