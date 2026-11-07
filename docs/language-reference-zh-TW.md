# SE 語言參考

[English version](language-reference.md)

這份 Reference 快速整理目前的使用者-facing 語言模型。想看完整教學請讀 [SE 完整教學](tutorial-zh-TW.md)；想了解 compiler 內部請讀 [技術參考](technical-reference-zh-TW.md)。

## Source 與註解

新的 source file 使用 `.se`：

```se
# 註解
say "Hello"
```

舊 `.s` 只保留 migration compatibility。

## Literals

```se
123
3.14
true
false
"text"
[1, 2, 3]
["name": "SE"]
set [1, 2, 3]
500ms
2s
1min
```

## 變數與 Assignment

```se
name = "SE"
count = 1
count += 1
count -= 1
count *= 2
count /= 2
count %= 3
```

Compound assignment 會沿用一般 assignment + binary operation 的語意。

## Operators

算術：`+ - * / % **`

比較：`== != > >= < <=`

邏輯：`and or not`

Membership：`in`

Range：`1..10`

大致 precedence 由高到低：

```text
**
unary - / not
* / %
+ -
..
comparison
== !=
and
or
```

`**` 是 right-associative。

## 條件判斷

```se
if score >= 90
    say "A"
else if score >= 80
    say "B"
else
    say "C"
```

較新的 source revision 也可能支援 `elif` 作為 `else if` alias；請以你實際 build 的 revision 為準。

## 迴圈

```se
repeat 3
    say "Hi"

for item in items
    say item

for key value in map_value
    say key
    say value

while running
    update
```

## 函式

```se
make add a b
    give a + b

answer = add 2 3
```

SE 採低標點呼叫。需要明確 grouping 時可使用括號。

## 型別標註與 Generic Function

```se
make identity[T] value:T -> T
    give value
```

能推斷時不必強制寫 annotation。

## User-defined Type

```se
type User
    name = ""
    score = 0

    make hello
        say name

user = User
    name = "SE"
```

每個 object 都有自己的 field storage。Method 內未被 local shadow 的 field 可以解析成 current object field，不需要強制到處寫 `self.`。

## Collections

```se
list = [1, 2, 3]
map_value = ["name": "SE"]
set_value = set [1, 2, 2, 3]

list.add 4
say map_value["name"]

if 3 in set_value
    say "found"
```

## Member 與 Index

```se
value.member
list[index]
map_value["key"]
value.help
```

支援的 value 可透過 `.help` 查看基本 member/help 資訊。

## Modules

```se
use path
```

Top-level 名稱以 `_` 開頭時遵循 module-private convention。Circular import 會被拒絕。

## 錯誤處理

```se
try
    text = read "data.txt"
    say text
else err
    say err.message
```

建立 runtime error：

```se
fail "message"
```

在支援的位置傳遞 fallible expression：

```se
make load
    give try read "data.txt"
```

## Match / Case

```se
match status
    case 200
        say "ok"
    case 404
        say "not found"
    else
        say "other"
```

目前 matching 以 value/equality 為主，不是完整 destructuring pattern system。

## Async

```se
job = async.run work 21
answer = async.await job
```

SE Web 對支援的 browser event `async.await` 也會降低成 JavaScript `await`。

## Web Source

```se
make Button text
    html
        button text

    css
        padding 12

    js
        when click
            say text

page "/"
    Button "Save"
```

建置：

```bash
se web build app.se dist
```

Browser request、routing 與表單請看 [Browser API](browser-api-0.8-zh-TW.md)。

## SE 0.8.0: 競賽輸入與開發工具

以下為 0.8.0 新增的正式 C++ 編譯器語法；輸入只讀取空白分隔資料，不會輸出提示文字。

```se
read n:Int
read a[n]:Int
sum = 0
for item in a
    sum += item
say sum

names[5]:Text = []
grid[10][10]:Int = []
```

可以使用 `se fmt file.se` 與 `se fmt --check file.se` 格式化、`se debug file.se` 除錯、`se trace file.se` 追蹤、`se lsp` 啟動實驗性語言伺服器、`se pkg init|add|remove|list|verify` 管理本機模組。套件管理尚未提供遠端套件註冊表或密碼學簽章。
