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
- **C++ backend**: Int, Num, Bool, Text, Bytes, homogeneous Lists, insertion-ordered Maps and Sets, common arithmetic/logic/conditions, inclusive range loops, recursion, indexed collection updates, functions, input/output and exceptions. List, Map, Set and Bytes assignments share backing storage; sorting and slicing return independent outer storage.
- **Recursive collection types**: indexed mutation, `for` loops over nested List/Set/Map types and `for key value in map` now use recursive type decomposition rather than a hard-coded collection-type whitelist.
- **Nested functions**: lexical nested `make` declarations compile to recursive callables. Captured local bindings use shared heap cells, including parameters and loop values; returning closures preserves their environment. Typed callables use type erasure so distinct branches can return compatible closures.
- **Generic user types**: `type Box[T]`, multi-parameter user types such as `Pair[A,B]`, specialized constructors (`Box[Int]`) and nested annotated fields are lowered to C++20 class templates with checked argument counts and field types.
- **Generic functions**: generic functions compile to C++ templates and first-class callable objects, so they can be passed to collection, function and task APIs. Same-type generic arithmetic and comparisons are accepted and checked again by C++ instantiation. Declared generic variables may use arbitrary names; repeated bindings, nested collection inference and nested generic functions have execution and rejection tests. This remains partial inference, not a full trait/constraint system.
- **Canonical bare-call arithmetic**: arguments absorb arithmetic (`twice a + b` means `twice(a+b)`), while `func a - 1 + func a - 2` splits into two calls when the next operand starts another call. Both have native/interpreter differential tests.
- **Block layout**: blank lines and comment-only lines between a block header and its first indented statement are accepted.
- **Local source modules**: `use helpers` recursively resolves `.se` / `.s` files relative to the importer and package locations. Module declarations receive stable prefixes; private names beginning with `_` remain private, distinct private helpers do not collide, and conflicting public exports are rejected. Imports preserve token adjacency and string contents. Source-module objects are not first-class runtime values.
- **C++ keyword safety**: SE identifiers such as `class`, `template` and `namespace` are escaped consistently during C++ generation.
- **Forward functions**: C++20 prototypes support explicit signatures and inferred return types in acyclic call chains with known operands. Return inference repeats until dependencies settle and leaves unresolved, heterogeneous, closure and loop-dependent returns to C++ deduction. Unannotated mutually recursive signatures still need work. `None` functions and methods may finish without `give`.
- **Library subset**: `math`, text and filesystem helpers; shared `collections` operations including contains/first/last/unique/keys/values/sort-by; `series` and `array`; `matrix` and `linear`; probability, fraction, complex, calculus, units, table and dataset helpers; JSON String/List[String] wire encoding; OS/time/random helpers; managed files; and test assertions. `extension_inventory.json` records 117 production extension modules and 586 advertised members. The bootstrap implements only the subset exercised by its runtime and differential tests; inventory entries do not imply parity.

## Native ABI

The SE-written loader now resolves `.snative` files and lowers metadata into
ordinary typed function AST wrappers. The independent runtime loads the native
library without the production VM. Int/Bool use C `int`, Num uses `double`, Text
uses copied NUL-terminated strings, Bytes uses the existing pointer/size view,
and tagged Handles retain their library and optional cleanup function. Mixed
argument signatures are generated directly from the declaration. Returned Bytes
are copied into owned storage; arguments are borrowed for the call. Missing
libraries/symbols, null results, tag mismatches and fallible integer failures
raise `NativeError`. Libraries remain loaded across calls, preserving native
module state. Cleanup symbols are validated before allocating a resource.

The native fixture requires `cmake --build build --target native_test`. Run
`c++ -std=c++20 -O0 -Iselfhost tests/selfhost_native_abi.cpp -o /tmp/se-abi-test`
and `/tmp/se-abi-test` for binary Bytes copying and Handle ownership checks.
Callbacks and arbitrary by-value C structs still need explicit C wrappers;
this does not establish complete extension-library compatibility.

The `.snative` grammar is one `library NAME` line and declarations of the form
`export_name c_symbol ArgType... -> ReturnType [fallible] [cleanup DESTROY]`.
Supported ABI types are `Int`, `Bool`, `Num`, `Text`, `Bytes`, `None` returns,
and `Handle:Tag`. `Int` is range-checked before conversion to C `int`; `Text`
uses NUL-terminated C strings, so embedded NUL requires a `Bytes` signature.
Native libraries load eagerly when imported and remain cached for the process.

The experimental backend supports an explicit `Any` type for scalar `Int`,
`Num`, `Bool`, `Text` and `Bytes` values. `dynamic.type_of` and `dynamic.is`
inspect these values, and `int`, `num` and `text` perform explicit conversions.
This bounded runtime variant does not erase arbitrary user objects or nested
containers. The `operator` module lowers its arithmetic and comparison helpers
with arity and operand-category checks.

### Additional extension API subset

The standalone runtime also implements the tested APIs in `iter` and
`itertools` (eager Lists), `url`, `encoding`, `config`, `dotenv`, `decimal`,
`datetime`, `data`, `enum`, `typing`, `functools`, `shutil`, `process`, and
`subprocess`, `option`, `result`, `match`, `queue`, `db`, `log`/`logging`, and
`glob`. `data` handles both statically typed collections and the tested dynamic
List/Map forms. Their conformance cases are in `selfhost/cases/` and run through
the generated C++ backend. The extension inventory is generated from the VS
Code API catalog. The current 2026-11-07 inventory detects lowerings for all 586
advertised members across 117 modules. The report keeps conformance separate:
the source scan does not prove complete production type, error, or runtime
parity, and the per-API executable cases remain authoritative.

## Added language coverage

- **Escaping closures**: shared lexical scalar bindings, mutation across repeated calls, independent factory instances, three nested scopes, loop captures and returned recursive functions. Untyped dynamic callback signatures still have less static checking than concrete signatures.
- **Generics**: nested generic functions, arbitrary type parameter names (`Item`, `Element`), generic methods and rejection of conflicting repeated bindings. Unknown type annotations and malformed collection annotations are rejected before C++ output.
- **Global bindings**: concretely inferred top-level variables are declared using deferred storage. Initialization remains in source order inside `main`; functions can read and mutate initialized bindings. Globals whose type remains `Unknown` are not promoted to this storage.
- **Managed tasks**: `async.run/await/ready` and `threading.run/join/ready` use independent C++ shared futures, including repeated awaits and exception propagation.
- **Function operations**: `function.bind`, `partial`, `call`, `map`, `filter`, and `reduce` work with native callable values. This is not full module coverage.
- **Component Web**: contextual `page`, `html`, `css/style`, `js`, `when`, native sections, HTML attributes/children, nested selectors, per-instance events and browser routing/API runtime. The SE backend emits real HTML/CSS/JS/TS assets. Tests compare the complete assets for `component-web.se` and `browser-api.se` against production (normalizing unordered route-registration order), syntax-check JavaScript, and reject invalid component arity/routes.

Build Web from the SE-written driver:

```sh
SE_SELFHOST_TARGET=web \
SE_SELFHOST_SOURCE=examples/browser-api.se \
SE_SELFHOST_OUTPUT=/tmp/se-web \
build/se run selfhost/compiler.se
```

After `python3 tests/selfhost_stage2.py build/se` passes, the verifier keeps
`build/se-selfhost` (or `.exe`) and its standalone C++ source. This executable
is the independently verified compiler from the real multi-file source graph, with no production SE VM linked in:

```sh
SE_SELFHOST_SOURCE=selfhost/cases/language_escaping_closures.se \
SE_SELFHOST_OUTPUT=/tmp/program.cpp \
build/se-selfhost
c++ -std=c++20 -O0 -Iselfhost /tmp/program.cpp -o /tmp/program
/tmp/program
```

The verification requires Python 3, a C++20 compiler and Node.js for JavaScript syntax checks.

The same environment variables work with the independently generated compiler executable. Native target requests containing Web constructs fail explicitly with instructions to select the Web target.

## Tests / 測試

```sh
se run selfhost/parser_test.se
se run selfhost/parser_extended_test.se
se run selfhost/typechecker_test.se
se run selfhost/typechecker_extended_test.se
se run selfhost/codegen_test.se
python3 tests/selfhost_differential.py $(command -v se)
python3 tests/selfhost_web.py $(command -v se)
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
5. Compile `selfhost/compiler.se` and its actual source modules directly from the native compiler, without the Python bundler, then recompile that same graph and compare generated C++ hashes.
6. Run the native differential and Web suites with this final multi-file compiler, then keep the verified executable and C++ source under `build/`.

A failure is a **failure**, never counted as a passing self-host result.
Local source imports and private-name isolation work in the tested subset, but first-class module values, many standard-library APIs, some nested-list aliasing rules and
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

- All generic inference combinations, generic callable type erasure, native ABI modules, complete recovery diagnostics, and all language forms. Generic `type`, generic methods/functions, lexical and escaping nested functions, managed tasks, component Web and value-based `match` are implemented in their documented tested forms.
- Complete standard library and platform/ABI compatibility; the production VM supports more modules than this bootstrap, and imported module availability is not equivalent to full API coverage.
- Comprehensive error recovery, Unicode/UTF-8 identifiers, source maps and source-accurate diagnostics.
- Broader self-host coverage: the strict Stage2/Stage3/Stage4 test passes for the currently bundled subset. Source bundling plus a production Stage0 is not proof by itself; changes must preserve native Stage1/2/3 self-compilation without linking or invoking the production VM after bootstrap.
- Cross-platform SE-only bootstrap on Linux/macOS/Windows is **not** verified. The fast differential suite currently runs on Linux only.

The production syntax/extension surface remains larger than this test corpus. These additions and passing tests must not be described as universal SE compatibility.

### Principle

Unsupported syntax must error instead of silently generating incorrect code. Compare both behavior and diagnostics against the production SE implementation before expanding the accepted language.

## Recursive types and annotated fields

`type_model.se` ports recursive type compatibility, generic binding and return-type substitution from the production checker. List/Map/Set C++ types are emitted recursively, including nested collection types. Repeated generic bindings are checked for conflicts. User-type fields support annotations, defaults, required initialization and checked field assignment. Generic user types are supported for their tested invariant specialization subset; advanced generic constraints and full type inference remain unfinished.

## Math, statistics and regular expressions

The bootstrap backend now maps the production math module's operational API to the independent runtime: constants pi/e/tau/inf, unary/binary functions, variadic hypot/min/max/gcd/lcm, interpolation, factorial/combinations/permutations, and population statistics. `statistics` provides mean/median and sample/population variance and deviation. `regex` and `re` provide match/search/replace/split using the same C++ ECMAScript regex implementation as production. Math combination overflow is explicitly rejected. Static module API checking and complete help output remain unfinished.

## Expression storage and numeric/control-flow semantics

Expression children are stored using internal `@ast:` handles into a shared
arena in `ast_codec.se`. Use `unpack_node` to inspect expressions; handles remain
valid within the compiler process and are not a portable serialized AST format.
Statement records and block encoding retain their JSON/Base64 structure.
This prevents repeated JSON escaping from growing exponentially with expression
depth. On the compiler bundle, the measured statement strings shrank from
542,009,153 bytes to 522,018 bytes (the source also includes the new inference pass).
The execution corpus includes an 80-term expression.

`%` supports Int and Num operands, evaluates operands from left to right, and
throws for zero divisors. Integer minimum remainder -1 produces zero without C++
undefined behavior. `repeat` evaluates its count once before entering the loop
and rejects negative counts. Numeric failure recovery has an explicit native
output oracle because the production interpreter currently throws a different
exception category for arithmetic errors.
