# SE Modules

[繁體中文版](modules-zh-TW.md)

SE uses `use` to load source, standard, package, or native modules through one module system.

## Basic import

```se
use math
say math.sqrt 25
```

For a local source file:

```text
app/
├── main.se
└── tools.se
```

`main.se`:

```se
use tools
```

## Resolution

The loader resolves modules through the project's source location, built-in/standard modules, package search paths, and native metadata where applicable. New configuration prefers `SE_HOME`; legacy `S_HOME` may remain as a fallback during migration.

## Public and private names

Top-level names beginning with `_` follow the private convention:

```se
_secret = 123
```

Other public top-level functions, types, and assigned names can be exported by the module. Name collisions are reported rather than silently choosing one definition.

## Dependency graph

The loader constructs the complete module dependency graph before execution or native code generation. Circular imports such as:

```text
a → b → a
```

are rejected with the dependency chain.

## Standard and platform modules

Available built-in/runtime modules depend on the current revision and include areas such as:

```text
file path time math random os
json text collections test process
http web js ts
function async option result match db https
statistics regex base64 uuid iter copy operator
```

Use the specific module reference/API documents for exact members.

## Native modules

Native modules are still imported with ordinary `use` syntax:

```se
use native_test
```

The external `.snative` metadata describes the C ABI so ordinary SE source does not need to contain ABI details.

## Source extension

New modules use `.se`. Legacy `.s` lookup may remain for migration compatibility, but new projects and documentation should use `.se`.
