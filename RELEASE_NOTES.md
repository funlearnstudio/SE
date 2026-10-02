# SE 0.7.5

Improve SE expression behavior and synchronize the CLI, installers, release packages, and VS Code extension.

- Support arithmetic inside no-parentheses bare function-call arguments, including `func a-1 + func a-2` and the whitespace-equivalent form.
- Allow Text concatenation with other printable SE values, such as `"hello" + 5`, `5 + " apples"`, Bool, and Num values.
- Keep numeric `+` behavior unchanged when both operands are numeric.
- Add regression coverage for bare-call Fibonacci syntax and mixed-type Text concatenation.
- Ship the VS Code extension as version 0.7.5 and package `se-language-0.7.5.vsix`.
- Update macOS/Linux `install.sh` and Windows `install.ps1` fallback versions to 0.7.5.
- Build release assets under the v0.7.5 tag for supported macOS, Linux, and Windows targets.
