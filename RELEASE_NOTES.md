# SE 0.7.0

SE 0.7.0 expands the standard library substantially and improves installation so the prebuilt packages can be used immediately across supported platforms.

## Quick install

macOS / Linux:

```sh
curl -fsSL https://raw.githubusercontent.com/funlearnstudio/SE/main/install.sh | sh
```

Windows PowerShell:

```powershell
irm https://raw.githubusercontent.com/funlearnstudio/SE/main/install.ps1 | iex
```

Both installers now resolve the latest GitHub Release automatically when `SE_VERSION` is not explicitly set.

## Standard library additions

SE 0.7.0 adds Python-inspired modules and aliases while keeping SE syntax and runtime behavior:

- `statistics`
- `iter` / `itertools`
- `regex` / `re`
- `decimal`
- `csv`
- `datetime` plus additional `time` helpers
- `hash` / `hashlib`
- `base64`
- `uuid`
- safe `pickle`-style JSON serialization
- `args` / `argparse`
- `log` / `logging`
- `shutil` plus additional `file` operations
- `glob`
- `zip` / `zipfile`
- `subprocess`
- `socket`
- `threading`
- `queue`
- `sqlite` / `sqlite3`
- `functools` plus additional `function` helpers
- `operator`
- `copy`
- `enum`
- `typing`

The new APIs are registered with the type checker and runtime, included in VS Code completion, and exercised by a dedicated standard-library smoke test.

## Installation improvements

The Unix installer checks optional host tools required by specific modules:

- `sqlite3` for `sqlite` / `sqlite3`
- `zip` and `unzip` for archive helpers
- `sha256sum` or `shasum` for SHA-256 hashing

To let the Unix installer attempt installation of those optional tools:

```sh
curl -fsSL https://raw.githubusercontent.com/funlearnstudio/SE/main/install.sh | SE_INSTALL_STDLIB_DEPS=1 sh
```

Supported package managers include Homebrew, apt, dnf, yum, pacman, apk and zypper.

## Compatibility notes

- `pickle` intentionally uses safe JSON serialization rather than Python's executable pickle format.
- `threading` uses SE's managed Task model.
- `decimal` currently uses long-double-backed textual decimal operations and is not arbitrary-precision Python Decimal parity.
- ZIP and SQLite helpers rely on host command-line tools.
- `se build` still requires a C++20 compiler; interpreted workflows do not.

## Distribution

The release workflow builds and verifies packages for:

- Linux x64
- macOS Intel
- macOS Apple Silicon
- Windows x64
- VS Code extension

Release assets include SHA-256 checksums.

## Version

SE 0.7.0
