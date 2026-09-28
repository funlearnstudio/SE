# Getting Started with SE

[繁體中文版](getting-started-zh-TW.md)

This guide gets SE installed, verifies the toolchain, and runs your first program. The current stable release is **SE 0.7.1**; a source checkout of `main` may contain newer compiler and Web work while reporting the 0.7 language version.

## 1. Install SE

### macOS / Linux

```bash
curl -fsSL https://raw.githubusercontent.com/funlearnstudio/SE/main/install.sh | sh
```

### Windows PowerShell

```powershell
irm https://raw.githubusercontent.com/funlearnstudio/SE/main/install.ps1 | iex
```

Verify:

```bash
se --version
se doctor
```

The prebuilt installation does not require CMake or a C++ compiler for the REPL, `run`, `check`, `check-all`, or `test`.

## 2. Build SE itself from source

Use this path when developing the language/compiler or when you want the newest source-tree features.

Requirements:

- Git
- CMake 3.20+
- a C++20 compiler

```bash
git clone https://github.com/funlearnstudio/SE.git
cd SE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/se --version
```

After a later `git pull`, rebuild with:

```bash
cmake --build build --parallel
```

## 3. Your first SE program

Create `hello.se`:

```se
say "Hello SE"
```

Run it:

```bash
se run hello.se
```

If you are using an uninstalled source build:

```bash
./build/se run hello.se
```

Static check without running:

```bash
se check hello.se
```

## 4. Native build

```bash
se build hello.se
```

The current native backend emits C++20 and invokes the system compiler, so this command requires a C++20 compiler even when SE itself was installed from a prebuilt package.

## 5. REPL

```bash
se
```

Example:

```text
> say "Hello"
Hello
```

A blank line completes an indented block.

## 6. Main CLI commands

```text
se
se --version
se help
se doctor
se check file.se
se check-all .
se run file.se
se test .
se build file.se
se new app myapp
se new web mysite
se web build app.se dist
se bind module.sbind generated
```

## 7. Create a project

Application:

```bash
se new app myapp
cd myapp
se check-all .
se test .
se run src/main.se
```

Web project:

```bash
se new web mysite
cd mysite
```

For component Web source:

```bash
se web build app.se dist
```

The component Web compiler outputs standard Web files such as `index.html`, `style.css`, `app.js`, and `app.ts`.

## 8. What to read next

1. [Tutorial](tutorial.md)
2. [Language Reference](language-reference.md)
3. [SE Web Language](web-language-0.8.md) if you want browser applications
4. [Technical Reference](technical-reference.md) if you want compiler/runtime internals

New source code should use `.se`. Legacy `.s` compatibility may remain in older examples or migration paths.
