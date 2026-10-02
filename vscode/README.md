# SE Language for Visual Studio Code

[繁體中文版](README-zh-TW.md)

Official Visual Studio Code support for SE source files (`.se`). Version 0.7.5 synchronizes module completion, member signatures, and syntax highlighting with the expanded SE runtime. The bundled syntax guide and live compiler diagnostics are included.

## Core features

- SE `.se` file recognition and TextMate syntax highlighting
- indentation support for SE blocks
- snippets for common language constructs
- completion for every current built-in module after `use `
- member IntelliSense for all current built-in runtime modules and aliases
- local completion for variables, functions and user-defined types
- module/member hover documentation and low-punctuation signatures
- hover information
- Go to Definition and Find References within the current file
- Outline / Breadcrumb symbols
- low-punctuation function signature help
- live `se check` diagnostics while typing, with red squiggles and Problems entries
- commands for running, terminal checking, Problems checking and building the current SE file
- bundled Traditional Chinese syntax guide via **SE: Open Syntax Guide**
- configurable path to the `se` executable and diagnostic delay

## Syntax highlighting

The grammar covers comments, strings/escapes, numeric and duration literals, operators, declarations, control flow, core types, standard modules and type annotations.

Grammar source:

```text
vscode/syntaxes/se.tmLanguage.json
```

TextMate scope:

```text
source.se
```

The same grammar is intended to remain suitable as the upstream grammar source for a future GitHub Linguist submission.

## IntelliSense

Prefix completion narrows the language-aware candidate set as you type. For example, `collections.` can offer collection helpers after importing the module.

```se
use collections

collections.
```

User-defined members can also be inferred from declarations in the current file:

```se
type Player
    name = ""
    hp = 100

    make hit damage
        hp -= damage

player = Player
player.
```

The extension can suggest `name`, `hp`, and `hit` from the local `Player` definition.

The expanded modules include data (`array`, `matrix`, `table`), formats (`toml`, `yaml`, `xml`, `markdown`), web (`http_server`, `websocket`, `session`), security (`crypto`, `jwt`), AI (`ai`, `ml`, `tensor`, `embedding`), and browser scenes (`canvas`, `sprite`, `camera`). Their editor completions match the functions currently implemented by the runtime. Some runtime operations need Python or an optional Python package; see the [module reference](../docs/expansion-modules-zh-TW.md) for their requirements and signatures.

The current editor analysis is intentionally lightweight and runs inside the extension. A future full LSP can reuse compiler/parser/checker semantics for deeper cross-file analysis without changing the user-facing editor model.

## Live diagnostics

The extension uses the real SE compiler/checker instead of duplicating the language rules in JavaScript. By default, an edited saved file is checked after a short debounce. Dirty editor contents are written to a temporary sibling `.se` file so imports continue resolving relative to the same project directory.

Compiler errors are mapped into VS Code Diagnostics and appear as red squiggles and in the **Problems** panel.

Settings:

- `se.diagnostics.enabled`
- `se.diagnostics.delay`
- `se.executablePath`

Use **SE: Check File Problems** to trigger the diagnostic checker immediately.

## Built-in syntax guide

Run **SE: Open Syntax Guide** from the Command Palette. The extension ships `TUTORIAL-zh-TW.md`, covering basic syntax, functions, types, modules, error handling, async/threading, SQLite, networking, tests, IntelliSense and diagnostics.

## Commands

From the Command Palette:

```text
SE: Run File
SE: Check File Problems
SE: Check File in Terminal
SE: Build File
SE: Open Syntax Guide
```

The extension expects `se` to be available in `PATH`. If it is installed elsewhere, configure **SE: Executable Path** in VS Code settings.

## Package locally

From the repository root:

```bash
cd vscode
npm install
npx vsce package
```

Install the generated `.vsix` with VS Code's **Install from VSIX...** command or the `code --install-extension` CLI.

The package filename/version follows the extension's current package metadata; do not hard-code an old version when following these instructions.

## Related documentation

- [SE Documentation](../docs/README.md)
- [GitHub Syntax Highlighting](../docs/github-syntax-highlighting.md)
