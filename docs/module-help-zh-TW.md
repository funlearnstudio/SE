# SE 套件與函式 .help 完整說明

從此版本的原始碼開始，SE 內建模組的 `.help` 會顯示**用途、匯入語法、實際匯出成員、參數格式、功能說明與參數個數**。函式值的 `.help` 會顯示該函式的語法與用途，無須查閱 VS Code 的 IntelliSense。

## 最基本的用法

```se
use file
use game
use math

file.help
game.help
math.help

say file.copy.help
say game.key_move.help
say math.sqrt.help
```

`module.help` 會列出**該模組實際匯出的 API**（而非僅根據文字宣告猜測）。輸出順序按函式名稱排序，方便查找。例如 `game.help` 中會列出 `game.new`、`game.sprite_color`、`game.key_move`、`game.animate`、`game.show` 等。

`module.function.help` 則提供該函式的使用範型、定義、最少/最多引數。例如：

```se
use game
say game.key_move.help
```

對照語法：

```se
screen = game.new 800 500 "Pong"
game.sprite_color screen "player" 50 100 20 80 "blue"

up = -10
game.key_move screen "player" "ArrowUp" 0 up
game.animate screen 60
try game.show screen
```

**重要：** 目前無括號呼叫語法中，`0 -10` 可能被解析為減法，建議先用 `up = -10` 將負數存入變數。

## 其他常見套件

```se
use file
file.help
say file.read.help
say file.write.help
say file.copy.help

use math
math.help
say math.factorial.help

use json
json.help
say json.stringify.help

use statistics
statistics.help
say statistics.mean.help

use sqlite3
sqlite3.help
```

## 欄位解釋

- **Purpose**：模組主要用途。
- **Import**：以 `use module` 匯入。
- **API reference**：`module.name` 與所需的參數位置；說明源自內建的文件目錄，並根據執行期實際 API 列出。
- **[arguments: N]**：至少要提供 N 個引數；`N..M` 代表可選引數，`N+` 代表可變長引數。
- **[value: Type]**：非函式成員的實際執行期型別。
- **Fallible**：可能失敗；依使用情境以 `try` 處理。
- **Example / Usage**：顯示語法範本；`path`、`number`、`scene` 等是參數名稱，使用時必須換成實際值。

## 覆蓋範圍及限制

內建目錄涵蓋 VS Code `language-data.js` 中定義的 **117 個套件**與 **541 個明確成員說明**。如函式的定義來自展開式產生、外部原生擴充或使用者自行建立模組，仍會根據執行時真實的 `exports` 顯示函式名稱與參數個數，但未必有詳細文字說明。

`ask`、`say` 等語言關鍵字不是獨立套件；一般 Text、List、Map 等值的 `.help` 維持原本功能。這是執行期說明，**不會執行被查詢的函式**。
