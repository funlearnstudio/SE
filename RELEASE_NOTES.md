# SE 0.7.3

SE 0.7.3 refreshes the prebuilt CLI packages from the current source so installations receive the expanded standard-library modules and runtime fixes. It also verifies release downloads before installing them.

## Windows installer fix

Correct PowerShell quote escaping when generating `se.cmd`, so the Windows installer can be parsed and run. CLI version output, installer fallback versions, and release package names are aligned at 0.7.3.

## Quick install

macOS / Linux:

```sh
curl -fsSL https://raw.githubusercontent.com/funlearnstudio/SE/main/install.sh | sh
```

Windows PowerShell:

```powershell
irm https://raw.githubusercontent.com/funlearnstudio/SE/main/install.ps1 | iex
```

Both installers resolve the latest GitHub Release automatically when `SE_VERSION` is not explicitly set. The Unix installer validates the requested version, verifies the package checksum, checks archive paths, and reports optional Python module dependencies.

## Standard library additions

The refreshed CLI packages include the current expansion APIs, including:

- data and math: `url`, `encoding`, `series`, `matrix`, `linear`, `dataset`, `table`, `probability`, `fraction`, `complex`, `calculus`, `units`
- content and security: `toml`, `yaml`, `xml`, `markdown`, `crypto`, `jwt`, `session`, `auth`, `cookie`, `cors`, `template`
- service and network: `http_server`, `router`, `dns`, `ftp`, `smtp`, `imap`, `ssh`, `websocket`, `ai`, `embedding`
- machine learning and media: `ml`, `tensor`, `image`, `audio`, `video`, `camera`, `gui`, `canvas`, `sprite`, `physics`, `sound`, `animation`, `scene`, `collision`, `tilemap`
- existing standard-library modules including `statistics`, `iter`, `itertools`, `regex`, `decimal`, `csv`, `datetime`, `hashlib`, `sqlite3`, and `functools`

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

SE 0.7.3
