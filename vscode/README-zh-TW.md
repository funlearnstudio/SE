# SE Language for Visual Studio Code

[English version](README.md)

這是 SE `.se` 原始碼的官方 Visual Studio Code 支援。0.7.3 將模組補全、成員簽章和語法上色同步至擴充後的 SE runtime，並提供內建語法教學與即時編譯器診斷。

## 核心功能

- `.se` file recognition 與 TextMate syntax highlighting
- SE block 縮排支援
- 常用語法 snippets
- 輸入 `use ` 時補齊目前所有內建模組
- 所有 current built-in runtime module / alias 的 member IntelliSense
- current file 內的 variable、function、user-defined type completion
- module/member hover 說明與 low-punctuation signature help
- hover information
- current file 內的 Go to Definition / Find References
- Outline / Breadcrumb symbols
- low-punctuation function signature help
- 輸入時自動執行 `se check`，錯誤直接顯示紅線與 Problems
- Run / Check Problems / Terminal Check / Build command
- **SE: Open Syntax Guide** 內建繁體中文語法教學
- 可設定 `se` executable path 與 diagnostics delay

## Syntax Highlighting

Grammar 涵蓋 comment、string/escape、number、duration literal、operator、declaration、control flow、core type、standard module 與 type annotation。

Grammar：

```text
vscode/syntaxes/se.tmLanguage.json
```

TextMate scope：

```text
source.se
```

這份 grammar 也應維持可作為未來 GitHub Linguist upstream submission 的 grammar source。

## IntelliSense

輸入 prefix 時，VS Code 會縮小 SE-aware candidate set。例如 import `collections` 後：

```se
use collections

collections.
```

可以提供對應 collection helper。

Current file 裡的 user-defined type member 也可以從 declaration 推斷：

```se
type Player
    name = ""
    hp = 100

    make hit damage
        hp -= damage

player = Player
player.
```

這時 extension 可以根據本機 `Player` definition 建議 `name`、`hp`、`hit`。

新模組涵蓋資料（`array`、`matrix`、`table`）、格式（`toml`、`yaml`、`xml`、`markdown`）、Web（`http_server`、`websocket`、`session`）、安全（`crypto`、`jwt`）、AI（`ai`、`ml`、`tensor`、`embedding`）及瀏覽器場景（`canvas`、`sprite`、`camera`）。編輯器只提示 runtime 已實作的函式。部分函式執行時需要 Python 或其套件，細節與簽章請看[模組參考](../docs/expansion-modules-zh-TW.md)。

目前 editor analysis 刻意保持 lightweight，直接在 extension 內執行。未來完整 LSP 可以重用 compiler/parser/checker 做更深入 cross-file semantic analysis，但不需要改變使用者-facing editor model。

## 即時自動偵錯

Extension 直接呼叫真正的 SE compiler/checker，不另外在 JavaScript 複製一套語言規則。輸入停止一小段時間後會自動檢查；尚未存檔的內容會暫時寫到原檔案同一個資料夾，因此 local module import 仍能以正確相對位置解析。

錯誤會顯示在：

- editor 紅色波浪線
- VS Code **Problems** panel
- diagnostic hover

設定：

- `se.diagnostics.enabled`
- `se.diagnostics.delay`
- `se.executablePath`

需要立刻重新檢查時，執行 **SE: Check File Problems**。

## 內建語法教學

Command Palette 執行 **SE: Open Syntax Guide**。Extension 會開啟內建 `TUTORIAL-zh-TW.md`，內容包含基礎語法、function、type、module、`try`、async/threading、SQLite、network、test、IntelliSense 與 diagnostics。

## Commands

Command Palette：

```text
SE: Run File
SE: Check File Problems
SE: Check File in Terminal
SE: Build File
SE: Open Syntax Guide
```

Extension 預設期待 `se` 已在 `PATH`。若安裝在其他位置，可設定 **SE: Executable Path**。

## 本機打包

從 repository root：

```bash
cd vscode
npm install
npx vsce package
```

接著使用 VS Code 的 **Install from VSIX...**，或 `code --install-extension` 安裝產生的 `.vsix`。

套件檔名與版本應以目前 package metadata 為準，不要在教學裡硬編一個之後會過期的版本號。

## 相關文件

- [SE 文件總覽](../docs/README-zh-TW.md)
- [GitHub Syntax Highlighting](../docs/github-syntax-highlighting-zh-TW.md)
