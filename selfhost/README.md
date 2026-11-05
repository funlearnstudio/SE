# SE Self-hosting Compiler Bootstrap (Experimental)

本目錄是用 **SE 語言本身**編寫 SE 編譯器的實驗分支，**不是**目前正式版的 C++20 編譯器。

## 目前已實作

1. `lexer.se` — 將原始碼切成 token；記錄 `[kind, lexeme, line, column]`，四欄均以 Text 儲存，符合 SE List 的元素同型別限制。
2. `parser.se` — Recursive Descent + Pratt precedence parsing。解析整數、浮點、字串、Bool、名稱、括號、單元與二元運算、賦值、`say`、`give`、`if/else`、`while`、`repeat`、不帶型別標註的 `make`。
3. `codegen.se` — 將上述語法子集的 AST 產生 C++20。
4. `bootstrap.se` — 串接 SE Lexer → Parser → Codegen，輸出 `selfhost/generated_sample.cpp`。
5. `parser_test.se` 與 `codegen_test.se` — 用 SE 寫的基本回歸測試，並在 CTest 註冊。

## 使用方式（從儲存庫根目錄執行）

```sh
se run selfhost/parser_test.se
se run selfhost/codegen_test.se
se run selfhost/bootstrap.se
c++ -std=c++20 -O0 selfhost/generated_sample.cpp -o selfhost/sample-selfhost
./selfhost/sample-selfhost
```

預期 sample 的輸出為 `Hello SE`。CLI 也可以先檢查：

```sh
se check selfhost/lexer.se
se check selfhost/parser.se
se check selfhost/codegen.se
```

## AST 格式

基於 SE 目前 List 的同型別元素限制，AST 使用 `List[Text]`，每筆節點以 JSON Text 表示：

- `["Atom", "Integer", "123", ""]`
- `["Binary", "+", "<left-json>", "<right-json>"]`
- `["Assign", "name", "<expr-json>", ""]`
- `["If", "<expr-json>", "<then-list-json>", "<else-list-json>"]`

這是 bootstrap 過渡格式，未來可換成真正的 `type` AST 與型別檢查器。

## 限制

- **尚未 self-hosting**：產生的 C++20 是簡單語法子集，而不是完整 SE compiler。
- 尚未有 Symbol Table / Type Checker、完整函式呼叫、`use`、List / Map / Set、泛型、例外處理、SE Web、原生 ABI。
- 此 codegen 使用 C++20 `auto` 進行型別推斷，和正式 SE 的數值／字串型別與動態運算語義未必完全相同。
- 同樣的變數名稱、作用域、函式回傳型別等需後續擴充。
- Lexer 以 UTF-8 byte split 實作，暫時主要支援 ASCII 的語法字元。
- 在 SE 的無括號函式呼叫中，負數與連續引數可能產生解析歧義；新的 bootstrap AST 只涵蓋部分形式。

## 後續順序

1. 確保 Lexer / Parser / C++20 Codegen smoke tests 在 Linux、macOS、Windows 全部通過。
2. 完成 AST nodes 與 Symbol Table、靜態型別檢查。
3. 支援所有核心 SE syntax 和 standard library，並對照 C++ compiler 做 differential tests。
4. 讓用 SE 寫的 compiler 編譯自身，再驗證 bootstrap equivalence。
