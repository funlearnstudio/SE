# SE 模組系統

[English version](modules.md)

SE 使用 `use` 透過同一套 module system 載入 source、standard、package 或 native module。

## 基本 Import

```se
use math
say math.sqrt 25
```

本機 source：

```text
app/
├── main.se
└── tools.se
```

`main.se`：

```se
use tools
```

## Resolution

Loader 會依專案 source 位置、built-in/standard module、package search path 與需要時的 native metadata 解析 module。新設定優先使用 `SE_HOME`；migration 期間可保留舊 `S_HOME` fallback。

## Public / Private 名稱

Top-level 名稱以 `_` 開頭時採 private convention：

```se
_secret = 123
```

其他公開 top-level function、type 與 assigned name 可由 module 匯出。若不同 import 造成公開名稱 collision，應回報錯誤，而不是靜默挑一個。

## Dependency Graph

Loader 會在 execution 或 native code generation 前建立完整 dependency graph。Circular import：

```text
a → b → a
```

會連同 dependency chain 一起被拒絕。

## Standard / Platform Modules

可用 built-in/runtime module 會依目前 revision 而異，主要包含：

```text
file path time math random os
json text collections test process
http web js ts
function async threading option result match db https
statistics regex re decimal csv datetime hash hashlib base64 uuid
iter itertools pickle args argparse log logging shutil glob zip zipfile
subprocess socket queue sqlite sqlite3 functools operator copy enum typing
```

每個 module 的精確 member 以對應 Reference / API 文件為準。

## Native Modules

Native module 仍用普通 `use`：

```se
use native_test
```

外部 `.snative` metadata 負責描述 C ABI，因此一般 SE source 不需要塞 ABI detail。

## Source Extension

新的 module 使用 `.se`。舊 `.s` lookup 可暫時保留 migration compatibility，但新專案與文件應一律使用 `.se`。
