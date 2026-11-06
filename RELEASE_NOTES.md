# SE 0.7.8

This release adds sized integer List declarations and synchronizes the compiler, the VS Code extension and the installation packages.

## Highlights

- New syntax: `a[5] = []` creates five zero-initialized integers.
- The length may be any Int expression, e.g. `n = 5` then `a[n] = []`.
- `a = []` still means an empty dynamic List; `a.add value` still grows it.
- Non-Int, negative, and oversized lengths are rejected; maximum 1,000,000 entries.
- Interpreter, native C++20 build, and SE Web code generation support sized list initialization.
- The VS Code extension ships a `sizedlist` snippet and updated Traditional Chinese syntax guide.

## Example

```se
a[5] = []
for i in 0..4
    a[i] = i + 1
say a
```

Expected output: `[1, 2, 3, 4, 5]`.

## Install

Install the new compiler and `se-language-0.7.8.vsix` from this GitHub Release. The editor extension does not replace the CLI; update both for the new syntax.
