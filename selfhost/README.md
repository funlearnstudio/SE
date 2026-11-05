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
typechecker.se         static checks: types, arity, undeclared names, returns
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
- **Parser**: expression precedence including unary and binary operators, list literals, indexing, member access, calls, `ask` and `try` expressions; statements `say`, `give`, `fail`, assignments including `+=` etc., `if/else if/else`, `while`, `repeat`, `for`, typed and untyped `make`, `use`, and multi-variable typed `ask`.
- **Static checks**: symbol existence, assignments, numeric and boolean operands, homogeneous lists, indexing, function arity, declared parameter types, annotated return types, selected block scopes, `ask` types and explicit failures.
- **C++ backend**: Int, Num, Bool, Text, nonempty homogeneous Lists, common arithmetic/logic/conditions, basic range loops, functions, simple module helpers, input/output and exceptions.
- **Library subset**: common `math`, `text`, `collections`, `file` and core conversion/read/write/append operations only. This **does not imply** general library compatibility.

## Tests / 測試

```sh
se run selfhost/parser_test.se
se run selfhost/parser_extended_test.se
se run selfhost/typechecker_test.se
se run selfhost/typechecker_extended_test.se
se run selfhost/codegen_test.se
python3 tests/selfhost_differential.py $(command -v se)
```

The differential test compiles generated C++20, runs the result, compares output with the production SE interpreter, and asserts that invalid programs are rejected. It covers arithmetic, functions, collections, for/while/repeat, string operations, typed multi-input and diagnostic failures.

A separately bootstrapped native compiler program can be produced from `selfhost/compiler.se` **using the production C++ SE compiler**. That binary may still link the production `s_core` VM, so it **does not prove true self-hosting or independent compiler runtime**.

## What is NOT finished / 尚待完成

- Complete parser and checker for `type`, Map/Set, generics, closures, `match`, async, native modules, SE Web and all language forms.
- Complete standard library and platform/ABI compatibility.
- Comprehensive error recovery, Unicode/UTF-8 identifiers, source maps and source-accurate diagnostics.
- Self-compile and reproducibility: compiler written in SE compiling **its own** sources, then stage2-stage3 binary equivalence.
- Cross-platform SE-only bootstrap verified on Linux/macOS/Windows. Differential tests currently verified in Linux; GitHub Actions needed for other platforms.

### Principle

Unsupported syntax must error instead of silently generating incorrect code. Compare both behavior and diagnostics against the production SE implementation before expanding the accepted language.
