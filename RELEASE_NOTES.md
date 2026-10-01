# SE 0.7.4

Fix native builds from installed SE packages on macOS and Linux.

- Include the native runtime sources and headers in each release package.
- Locate that bundle relative to the running SE executable, including when launched through the installer symlink.
- Keep source-checkout builds working while preventing installed binaries from using the release runner checkout.
- Report a clear reinstall message when the native runtime bundle is missing.
- Verify native compilation from the extracted release archive in relocated paths containing spaces, and verify the resulting executable.
- Retain the PowerShell installer quote fix from 0.7.3.

`se build` still requires a compatible C++20 compiler. The new native package regression runs on Linux and both macOS architectures; Windows package interpreter checks remain in place.

The VS Code extension remains at 0.7.3.
