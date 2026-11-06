# SE 完整教學

[English version](tutorial.md)

SE 的核心原則是 **Simple at every level.**：使用縮排、低標點函式呼叫、型別推斷、安全的 runtime 操作，並盡量讓直譯、Native 與 Web 開發共用熟悉的語言模型。

這份教學專注在「怎麼寫 SE」。如果要了解 compiler 內部，請看 [技術參考](technical-reference-zh-TW.md)。

## 1. Hello SE

```se
say "Hello SE"
```

執行：

```bash
se run hello.se
```

只檢查、不執行：

```bash
se check hello.se
```

## 2. 變數與值

SE 會從值推斷常見型別：

```se
name = "Steve"
age = 15
score = 98.5
ready = true
```

常見 runtime value family 包含 `None`、`Int`、`Num`、`Bool`、`Text`、`Bytes`、`List`、`Map`、`Set`、Function、Object、Error、Duration、Path/File 與 managed native handle。

## 3. 輸入與輸出

```se
name = ask "Your name?"
say "Hello " + name
```

`say` 輸出值；`ask` 顯示提示並取得文字輸入。

## 4. 數學、比較與邏輯

```se
a = 10
b = 3

say a + b
say a - b
say a * b
say a / b
say a % b
say a ** 2

say a > b
say a == b
say a != b
say ready and a > b
say not ready
```

## 5. 條件判斷

```se
score = 85

if score >= 90
    say "A"
else if score >= 80
    say "B"
else
    say "C"
```

SE 使用縮排形成 block，不使用 `{}`。較新的 source branch 也可能提供 `elif` alias；請以你實際 build 的 revision 為準。

## 6. 迴圈

固定次數：

```se
repeat 3
    say "Hi"
```

遍歷 collection：

```se
nums = [1, 2, 3]

for n in nums
    say n
```

條件迴圈：

```se
n = 0
while n < 3
    say n
    n += 1
```

Range：

```se
for n in 1..10
    say n
```

## 7. 函式

函式使用 `make`，回傳使用 `give`：

```se
make add a b
    give a + b

answer = add 5 3
say answer
```

SE 刻意減少函式呼叫時不必要的括號。

函式也可以成為值與 closure：

```se
make make_adder base
    make inner value
        give base + value
    give inner

add10 = make_adder 10
say add10 5
```

## 8. List、Map、Set

List：

```se
nums = [1, 2, 3]
nums.add 4
say nums.len
say nums[0]
```

Map：

```se
user = ["name": "Steve", "role": "student"]
say user["name"]

for key value in user
    say key
    say value
```

Set：

```se
values = set [1, 2, 2, 3]
values.add 4

if 3 in values
    say "found"
```

SE 會檢查不合法的 collection 操作，而不是把 C++ undefined behavior 暴露給一般 SE 程式。

### 預先配置固定數量的整數元素

需要先建立 5 個位置、之後才輸入資料時，可以寫：

```se
a[5] = []             # 建立長度為 5 的 List[Int]；初值全為 0
for i in 0..4
    a[i] = int ask "請輸入數值："
say a
```

長度也可以是 Int 變數：

```se
n = int ask "陣列長度："
a[n] = []
say a.len
```

這裡的 `a[n] = []` 是**陣列建立／重新初始化語法**，不是對第 n 個元素賦值。
`a = []` 仍建立可透過 `a.add value` 擴充的空陣列。新語法初始元素是整數 `0`，索引從 0 開始；長度不得為負，也不能大於 1,000,000。若需其他元素型別，請用一般 List literal 或 `.add` 建立。


## 9. Text 與 Bytes

```se
text = "Hello"
say text.len
say text.upper
say text.lower
```

Binary-safe 資料使用 `Bytes`：

```se
data = bytes "hello"
say data.len
```

Text 與 Bytes 分開，讓 binary API 不必把任意 bytes 假裝成文字。

## 10. 自訂 Type 與 Method

```se
type Player
    name = ""
    hp = 100

    make hit damage
        hp = hp - damage

    make alive
        give hp > 0

player = Player
    name = "Steve"

player.hit 20
say player.hp
```

Method 可以直接存取 field；一般 SE 程式不必每次都強制寫 `self.`。

## 11. Modules

`player.se`：

```se
type Player
    hp = 100
```

`main.se`：

```se
use player

p = Player
say p.hp
```

新程式使用 `.se`。Top-level 名稱以 `_` 開頭時遵循 module-private convention。Circular import 會被拒絕。

## 12. 錯誤處理

處理可恢復錯誤：

```se
try
    text = read "data.txt"
    say text
else err
    say err.message
```

建立錯誤：

```se
make check score
    if score < 0
        fail "Invalid score"
    give score
```

傳遞 recoverable error：

```se
make load
    give try read "data.txt"
```

## 13. Pattern Matching

SE 提供 value-based `match` / `case`：

```se
match status
    case 200
        say "ok"
    case 404
        say "not found"
    else
        say "other"
```

目前是值／相等比較型 matching，不是完整 destructuring pattern system。

## 14. Option 與 Result

進階 SE 提供 Option / Result helper，用來表示「可能沒有值」或「成功／失敗」的資料層結果。當你希望錯誤狀態本身是值，而不是控制流程 error 時，可以使用它們。

API 範例請看 [SE 0.6 進階功能](advanced-0.6-zh-TW.md)。

## 15. Async Task

```se
job = async.run slow_work 21
answer = async.await job
say answer
```

Task 是 managed runtime task；不要把它理解成任意 SE VM 程式都能無限制平行執行。

## 16. File、Path、Time

File：

```se
write "hello.txt" "Hello"
append "hello.txt" " SE"
text = read "hello.txt"
say text
```

Path：

```se
use path
file = path.join "data" "user.txt"
say path.exists file
```

Time / Duration：

```se
use time
say time.now
wait 500ms
wait 2s
```

## 17. JSON 與 HTTP

JSON object 會對應 Map、array 對應 List、string 對應 Text、數字對應數值型別、boolean 對應 Bool、null 對應 None。

Backend HTTP：

```se
use http
body = http.get "http://example.com/"
say body
```

HTTPS 也可使用；目前 TLS transport 會委派給系統 `curl`。

## 18. 測試

測試檔慣例使用 `*_test.se`：

```se
use test

test.equal 4 2 + 2
test.ok true
```

執行專案測試：

```bash
se test .
```

## 19. SE Web

Web component 直接沿用 `make`：

```se
make Button text
    html
        button text

    css
        padding 12
        border_radius 8

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

輸出是標準 HTML、CSS、JavaScript/TypeScript。完整說明請看 [SE Web Language](web-language-0.8-zh-TW.md)。

## 20. Browser API 與多頁面

SE Web 可以直接做 browser-side request 與頁面導航：

```se
make LoadUsers
    html
        button "Load"

    js
        when click
            try
                options = ["timeout": 8000, "retries": 2]
                task = browser.get_json "/api/users" options
                result = async.await task
                say result.data
            else err
                say err.message

page "/"
    LoadUsers

page "/about"
    About
```

使用 `browser.go` 切換頁面、`browser.post_json` 傳 JSON、`browser.cancel` 取消有 key 的 request。詳細 API 請看 [Browser API](browser-api-0.8-zh-TW.md)。

## 21. Native Interoperability

Native library 透過 C ABI 描述，而不是直接把任意 C++ ABI 暴露給 SE。一般 SE 程式仍然正常 import：

```se
use native_test
say add 20 22
```

Native bridge 支援 scalar、Text、Bytes 與 managed opaque handle。詳細內容請看 [Native Interoperability](native-interop-zh-TW.md)。

## 22. Interpreter、Native、Web 工作流程

```text
se check app.se          靜態檢查
se run app.se            Interpreter
se build app.se          C++20 Native Backend
se web build app.se dist Web Compiler
```

SE 的目標不是讓不同 target 變成不同語言，而是讓你換執行方式時仍保留熟悉的語法與概念。

## 下一步

- [語言參考](language-reference-zh-TW.md)：查精確語言規則。
- [技術參考](technical-reference-zh-TW.md)：了解 compiler/runtime。
- [SE Web Language](web-language-0.8-zh-TW.md)：學 Web component。
- [Browser API](browser-api-0.8-zh-TW.md)：學 API、表單與 routing。
