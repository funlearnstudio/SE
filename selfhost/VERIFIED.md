# Verified self-hosting checkpoint — 2026-11-06

The compiler frontend, source-module loader and native/Web backends are written
in SE. The generated native compilers use only the standalone bootstrap header
and C++ standard library after Stage0.

## Completed verification

- Strict Stage2 → Stage3 → Stage4 recompilation: identical generated C++.
- Direct compilation of `selfhost/compiler.se` and its actual source modules,
  followed by another compilation of that same graph: identical generated C++.
  This direct compilation does not use the Python source bundler.
- Final native multi-file compiler: 45 positive execution cases and 25 rejected
  invalid programs, plus 6 SE compiler unit suites.
- Web: complete asset comparisons for `component-web.se` and `browser-api.se`,
  JavaScript syntax checks, and 3 invalid component/route cases.
- Standalone runtime tests and 6 targeted CTest checks passed.
- Escaping closure execution also passed AddressSanitizer/UndefinedBehaviorSanitizer
  checks. LeakSanitizer was disabled because this executor uses ptrace.

Generated C++ SHA-256:

| Proof | SHA-256 |
| --- | --- |
| Stage2/3/4 bundle | `a2cb8a75e767d3537ae095a2afc42f5a0cd08344a626deb088d46a39c3b92843` |
| Direct multi-file generations | `072fc8138bc3d66323d7b171fc038a07f21c87a1539abb94e32db4fdd16dc257` |

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
