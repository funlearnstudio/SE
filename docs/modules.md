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
function async threading option result match db https
statistics regex re decimal csv datetime hash hashlib base64 uuid
iter itertools pickle args argparse log logging shutil glob zip zipfile
subprocess socket queue sqlite sqlite3 functools operator copy enum typing
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


## Independent package APIs

Every built-in name is a module in its own right. Related packages can share internal helpers while exposing focused APIs. For example, `re` provides match extraction, `itertools` provides sequence transforms, `config` parses sectioned settings, `series` handles rolling numeric data, `linear` provides matrix operations, and `dataset` provides row selection and splitting. Game packages such as `canvas`, `sprite`, `physics`, and `collision` expose separate parts of the scene API.
