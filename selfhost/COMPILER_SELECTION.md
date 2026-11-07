# Choose SE compiler backend / 選擇 SE 編譯器

Both compiler implementations remain separate:
- **CMake/C++ production SE**: `build/se`, created by CMake; authoritative full compiler/runtime.
- **SE-written self-hosted compiler**: `build/se-selfhost`, created only after the strict Stage2/3/4 verification; experimental subset; outputs independent C++20 or Web assets.

Use the explicit launcher (Python 3):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target se -j2
python3 tools/se_compiler.py --backend cmake check examples/hello.se
python3 tools/se_compiler.py --backend cmake run examples/hello.se
python3 tools/se_compiler.py --backend cmake build examples/hello.se
python3 tools/se_compiler.py --backend cmake web examples/component-web.se

# Independent bootstrap verification is required before selecting SE backend.
python3 tests/selfhost_stage2.py build/se
python3 tools/se_compiler.py --backend se compile selfhost/cases/language_core.se -o /tmp/core.cpp
python3 tools/se_compiler.py --backend se run selfhost/cases/language_core.se
python3 tools/se_compiler.py --backend se web examples/component-web.se -o /tmp/se-web
```

Select binaries explicitly with `--cmake-bin` and `--selfhost-bin`, or set
`SE_CMAKE_BIN` and `SE_SELFHOST_BIN`. The launcher **never falls back**
from SE self-host to CMake on an error. `--backend se check/build` deliberately
fails because the independent backend does not implement those production CLI
commands. SE-generated native C++ requires a host C++20 compiler and the
standalone header in `selfhost/`. `run` compiles temporary C++20 using `CXX`
(default `c++`), then executes it.

Backends are **implementations, not equivalent language versions**. A green
Stage2/3/4 bootstrap only proves self-compilation for the verified subset,
not parity with the complete production language and its 117+ modules.

Launcher regression suite (runs without either compiled backend):

```sh
python3 -m unittest discover -s tests -p test_se_compiler_selector.py -v
```

Main and the experimental branch have diverged. Do not merge without reconciling
upstream changes and passing native, selfhost, Web, and cross-platform CI tests.

## API lowering and runtime parity audit / API 完整度

The independent compiler now includes standalone, test-fixtured lowerings for
`xml.escape`, `json.pretty` (basic homogeneous JSON values),
`hash.sha256`, `hash.file_sha256`,
`hashlib.sha256`, `hashlib.file_sha256`,
`hashlib.hmac_sha256`,
`crypto.sha256`, `crypto.hmac_sha256`, and
`crypto.constant_time_equal`. The SHA-256/HMAC code is native C++20,
self-contained and includes reference test vectors. Extended API and
input-type parity has not been certified; implementation does not imply full
production behavior.

Inspect the exhaustive 117-module, 586-member inventory with:

```sh
python3 tools/selfhost_api_audit.py
python3 tools/selfhost_api_audit.py --json
python3 tools/selfhost_api_audit.py --require-complete
```

`--require-complete` deliberately returns nonzero until **every** API has
a lowering **and** independently certified runtime parity. Do not label this
experimental compiler as full SE or produce a stable combined release
until these checks, the bootstrap proof, and cross-platform tests pass.
