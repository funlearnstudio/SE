# SE 0.7.6

This release synchronizes the native SE CLI, documentation, and VS Code extension.

## Highlights

- Support typed multi-value input: `ask a:Int b:Int` and `ask name:Text age:Int height:Num`.
- Read whitespace-separated values on one line, validate their count and numeric types.
- Preserve the existing `name = ask "Question"` expression.
- Add an `askmulti` VS Code snippet and diagnostics support.
- Package SE Language 0.7.6 VSIX plus macOS, Linux and Windows binaries.

## Example

```se
ask a:Int b:Int
say a + b
```

Input `10 20` produces `30`.

## Install

Download an archive for your platform or `se-language-0.7.6.vsix` from this release.
