# SE 語法教學

這份教學會隨 SE VS Code extension 一起安裝。你可以從 Command Palette 執行 **SE: Open Syntax Guide** 隨時打開。

## 1. 第一支程式

```se
say "Hello SE"
```

`say` 會輸出值。SE 使用縮排表示區塊，通常不需要分號、大括號或函式呼叫括號。

讀取輸入：

```se
name = ask "你的名字？"
say "Hello " + name
```

註解：

```se
# 這是註解
say "SE"
```

## 2. 變數與基本型別

```se
name = "Steve"      # Text
age = 16            # Int
score = 98.5        # Num
ready = true        # Bool
```

常用轉型：

```se
a = int "123"
b = num "3.14"
c = text 123
d = bool "true"
e = char 65
```

## 3. List、Map、Set

List：

```se
scores = [90, 95, 100]
say scores[0]
say scores.len
scores.add 80
```

Map 的 key 使用 Text：

```se
student = ["name": "Steve", "grade": "10"]
say student["name"]
```

Set 會保留唯一值：

```se
values = {1, 2, 2, 3}
say values.len
```

## 4. if / else

```se
score = 90

if score >= 60
    say "pass"
else
    say "fail"
```

邏輯運算：

```se
if score >= 60 and score <= 100
    say "valid"
```

可使用 `and`、`or`、`not`。

## 5. 迴圈

固定次數：

```se
repeat 3
    say "Hello"
```

for：

```se
for n in 1..5
    say n
```

List：

```se
names = ["A", "B", "C"]

for name in names
    say name
```

while：

```se
n = 0

while n < 5
    say n
    n += 1
```

## 6. 函式

用 `make` 宣告函式，用 `give` 回傳值：

```se
make add a b
    give a + b

answer = add 10 20
say answer
```

SE 採低標點呼叫語法：

```se
say add 1 2
```

也可以使用型別標註：

```se
make double value:Int -> Int
    give value * 2
```

## 7. 自訂 type

```se
type Player
    name = ""
    hp = 100

    make hit damage
        hp -= damage

player = Player
player.name = "SE"
player.hit 10
say player.hp
```

VS Code extension 會分析目前檔案中的 field 與 method，因此輸入：

```se
player.
```

會提供 member completion。

## 8. match / case

```se
value = 2

match value
    case 1
        say "one"
    case 2
        say "two"
    else
        say "other"
```

## 9. use 模組

```se
use math
use statistics
use json
```

輸入 `use ` 後，VS Code 會列出目前所有內建模組。

例如：

```se
use math

say math.sqrt 25
say math.pow 2 8
```

統計：

```se
use statistics

nums = [10, 20, 30, 40]
say statistics.mean nums
say statistics.median nums
```

Regex：

```se
use regex

say regex.search "[0-9]+" "SE2026"
```

JSON：

```se
use json

data = ["name": "SE", "version": "1"]
encoded = json.stringify data
say encoded
```

## 10. 目前 built-in modules

基礎與資料：

```text
file path time math random os
json text collections data test
```

函式、錯誤與並行：

```text
function functools operator copy
option result match
async threading queue
typing enum
```

Python-style compatibility：

```text
statistics decimal csv datetime
regex re
hash hashlib base64 uuid
iter itertools
pickle
args argparse
log logging
shutil glob zip zipfile
subprocess socket
sqlite sqlite3
```

Web / 外部工具：

```text
process http https net web
js ts node next game db
```

擴充模組：

```text
url encoding dotenv config toml yaml xml markdown
table dataset array series matrix linear calculus complex fraction probability units
crypto jwt session auth cookie cors http_server router template static upload
websocket dns ftp smtp imap ssh email
ai ml tensor embedding image audio video camera
gui window canvas input sprite physics sound keyboard mouse animation scene collision tilemap
```

例如計算矩陣乘積，並在輸入 `matrix.` 後查看可用函式：

```se
use matrix

result = try matrix.multiply [[1, 2], [3, 4]] [[5, 6], [7, 8]]
say result
```

`try` 用於可能失敗的函式。`canvas`、`sprite` 等別名含 `game` 的場景函式，並額外提供動畫、碰撞、音效等成員；`video` 與 `camera` 各有專屬的開始／停止函式。各模組目前實作的 API 與執行需求請參閱專案的 `docs/expansion-modules-zh-TW.md`。

常用 aliases：

```text
re        -> regex
itertools -> iter
hashlib   -> hash
argparse  -> args
logging   -> log
zipfile   -> zip
sqlite3   -> sqlite
```

## 11. fallible 操作與 try

有些 API 可能失敗，例如檔案、網路、Base64 decode、SQLite query。這些呼叫需要 `try`。

Expression 形式：

```se
use base64

encoded = base64.encode "SE"
decoded = try base64.decode encoded
say decoded
```

Block 形式：

```se
try
    text = read "data.txt"
    say text
else err
    say err.message
```

VS Code 自動偵錯器會直接使用 SE compiler/checker，因此忘記 `try`、型別錯誤、語法錯誤、未知 member 等都會顯示在 **Problems**。

## 12. File

```se
use file

text = try file.read "data.txt"
try file.write "output.txt" text
try file.mkdir "backup"
try file.copy "output.txt" "backup/output.txt"
```

## 13. CSV

```se
use csv

text = "name,score\nSteve,100"
rows = csv.parse text
say rows.len
```

## 14. datetime / time

```se
use datetime

now = datetime.timestamp
say datetime.from_timestamp now
say datetime.format now "%Y-%m-%d"
```

```se
use time

now = time.now
say time.iso now
```

## 15. async / threading

```se
use async

make double x
    give x * 2

task = async.run double 21
answer = try async.await task
say answer
```

Managed threading：

```se
use threading

task = threading.run double 21
answer = try threading.join task
say answer
```

## 16. queue

```se
use queue

q = queue.new
queue.put q "A"
queue.put q "B"

say queue.size q
say try queue.get q
```

## 17. SQLite

需要系統中有 `sqlite3`：

```se
use sqlite3

db = sqlite3.open "school.db"
try sqlite3.exec db "CREATE TABLE IF NOT EXISTS students (name TEXT, score INTEGER)"
rows = try sqlite3.query db "SELECT * FROM students"
say rows
```

## 18. HTTP / HTTPS / net

HTTP：

```se
use http

body = try http.get "http://example.com"
say body
```

HTTPS：

```se
use https

body = try https.get "https://example.com"
say body
```

`https` / `net` 目前依賴系統 `curl`。

## 19. 測試

建立 `*_test.se`：

```se
use test

value = 2 + 2
test.equal value 4
test.ok value == 4
```

執行：

```bash
se test .
```

## 20. VS Code 自動偵錯

Extension 預設在你輸入後約 450ms 自動執行 checker。

可以在 Settings 調整：

- **SE › Diagnostics: Enabled**：開關自動診斷
- **SE › Diagnostics: Delay**：輸入後延遲多久檢查
- **SE: Executable Path**：指定 `se` 執行檔位置

錯誤會出現在：

- 編輯器紅色波浪線
- Problems panel
- Hover error message

也可以手動執行：

```text
SE: Check File Problems
```

若想在 Terminal 看完整 checker 輸出：

```text
SE: Check File in Terminal
```

## 21. 常用 Command Palette 指令

```text
SE: Run File
SE: Check File Problems
SE: Check File in Terminal
SE: Build File
SE: Open Syntax Guide
```

## 22. 使用 IntelliSense

輸入：

```se
use 
```

會建議所有 built-in modules。

Import 後輸入：

```se
use statistics

statistics.
```

會看到 `mean`、`median`、`variance`、`stdev` 等 API，並顯示呼叫方式與說明。

同樣適用於 `sqlite3.`、`regex.`、`queue.`、`game.` 等內建模組。
