# SE 0.8.0 — Language and Developer Tooling

SE 0.8.0 adds substantial functionality to the production C++ SE compiler and its VS Code extension while retaining indentation-based syntax.

## Language improvements

- **Silent contest input**: `read n:Int`, `read number:Num`, `read word:Text`, `read flag:Bool`, and `read values[n]:Int`. Tokens are read across spaces and line breaks with no prompt text.
- **Typed arrays**: `a[5]:Text = []`, `b[5]:Num = []`, `flags[5]:Bool = []`, zero-initialized to their types.
- **Multidimensional arrays**: `grid[10][10]:Int = []` with independent rows, static type checks, bounds checks and bounded allocation.
- Existing `a[5] = []` remains supported; the dynamic `a = []` / `a.add value` behavior is unchanged.

## Development tools

- `se fmt file.se` / `se fmt --check file.se`: conservative, idempotent whitespace formatting.
- `se debug file.se`, `se debug --break 5 file.se`, `se trace file.se`: interactive step/continue, locals, simple call stack and execution tracing.
- `se lsp`: Node.js-based standalone language-server foundation offering document symbols, workspace navigation, completion, references, rename, formatting and compiler-backed diagnostics.
- `se pkg init|add|remove|list|verify`: project-local SE module packages, with lockfile change detection. Supports local files only; no public registry or remote package fetching.
- VS Code editor-formatting support, new syntax snippets and updated guides.

## Native compiler

- Adds a **limited direct C++ lowering path** for straightforward, side-effect-free sequential assignments, arithmetic and output.
- Complex control flow, modules, functions and dynamic operations continue to use the established interpreter-backed native runtime. This is **not a completed optimizing Native Backend**.

## Scope and limitations

- The standalone LSP and CLI debugger are foundations, not yet a full scope-aware language server or DAP debugger.
- Package-lock checksums are not cryptographic signatures.
- The separate SE self-hosting compiler and full native/generic feature-parity effort remain ongoing, not completed in this release.
- Cross-platform CI and interpreter/native parity tests must pass before publishing.

## Quick example

```se
read n:Int
read values[n]:Int
total = 0
for value in values
    total += value
say total
```

For the editor, install `se-language-0.8.0.vsix` from this release. Updating the extension does not replace the installed SE CLI.
