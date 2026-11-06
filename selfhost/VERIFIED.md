# Verified self-hosting checkpoint — 2026-11-06

The compiler frontend, source-module loader and native/Web backends are written
in SE. The generated native compilers use only the standalone bootstrap header
and C++ standard library after Stage0.

## Completed verification

- Strict Stage2 → Stage3 → Stage4 recompilation: identical generated C++.
- Direct compilation of `selfhost/compiler.se` and its actual source modules,
  followed by another compilation of that same graph: identical generated C++.
  This direct compilation does not use the Python source bundler.
- Final native multi-file compiler: 64 positive execution cases and 25 rejected
  invalid programs, plus 6 SE compiler unit suites.
- Web: complete asset comparisons for `component-web.se` and `browser-api.se`,
  JavaScript syntax checks, and 3 invalid component/route cases.
- Standalone runtime tests and 6 targeted CTest checks passed.
- Escaping closure execution also passed AddressSanitizer/UndefinedBehaviorSanitizer
  checks. LeakSanitizer was disabled because this executor uses ptrace.

Generated C++ SHA-256:

| Proof | SHA-256 |
| --- | --- |
| Stage2/3/4 bundle | `17193a972971e5ee5c8b8b7b799e1699eae2e632636f8fa70f86057312046f15` |
| Direct multi-file generations | `4ddd8ae8de2679c32cc22a60460216c7bad65f17dd04c5ec868b376f4f8ee776` |

Reproduce on Linux with Python 3, C++20 and Node.js:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target se --parallel 2
python3 tests/selfhost_stage2.py build/se
```

After success, use the exported independent `build/se-selfhost` executable.
It accepts `SE_SELFHOST_SOURCE`, `SE_SELFHOST_OUTPUT` and the optional Web target
`SE_SELFHOST_TARGET=web`, as documented in README.md.

## Scope limits

This proof and corpus do not establish universal SE compatibility. Native ABI
modules and the complete standard/extension library remain unfinished.
First-class module values, all generic callable combinations, full dynamic
collection/object aliasing, comprehensive diagnostics and cross-platform
self-hosting still need work. Source modules have tested private-name isolation,
public exports, duplicate-export rejection and `.se` / `.s` package resolution.

## Latest checkpoint: 209ac2a

The strict proof above was rerun after CSV read/write, expression arena storage,
inferred acyclic forward signatures, implicit `None` returns, numeric remainder,
and single-evaluation repeat counts. All 64 execution cases, the eager native
load failure case, 25 rejected programs, 6 SE compiler unit groups, and the Web
corpus passed with the final direct-module standalone compiler. The standalone
runtime also passed UndefinedBehaviorSanitizer checks, including signed minimum
remainder -1 and zero divisors.

Expression statement-record bytes in the bundled compiler dropped from
542,009,153 to 522,018. This measures the serialized outer statement records,
not total process memory; the arena stores additional expression records.
An 80-term expression is included in the execution corpus.

This checkpoint remains incomplete. Unannotated mutual recursion, unresolved
loop-dependent or heterogeneous return inference, first-class modules, full
object/container `Any`, all generic callable combinations, complete extension
coverage, and cross-platform ABI verification are not certified by this proof.
