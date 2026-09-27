# SE

> Simple at every level.

SE is a low-punctuation, safety-first programming language implemented in C++20. It aims to keep the path from a first program to larger applications readable and consistent.

SE 是一門以 C++20 實作、低標點、重視安全性的程式語言。它的目標是讓程式從第一行一路成長到較大型應用時，仍保持一致、可讀與簡單。

```text
Language / 語言: SE
CLI / 指令:       se
Source / 原始碼:  .se
Release / 正式版: SE 0.7.0
```

```se
name = ask "Your name?"

if name == "SE"
    say "Hello SE"
else
    say "Hello " + name
```

## Start here / 從這裡開始

| English | 繁體中文 |
| --- | --- |
| [Documentation](docs/README.md) | [文件總覽](docs/README-zh-TW.md) |
| [Getting Started](docs/getting-started.md) | [快速開始](docs/getting-started-zh-TW.md) |
| [Tutorial](docs/tutorial.md) | [完整教學](docs/tutorial-zh-TW.md) |
| [Language Reference](docs/language-reference.md) | [語言參考](docs/language-reference-zh-TW.md) |
| [Technical Reference](docs/technical-reference.md) | [技術參考](docs/technical-reference-zh-TW.md) |
| [SE Web](docs/web-language-0.8.md) | [SE Web](docs/web-language-0.8-zh-TW.md) |
| [Browser API](docs/browser-api-0.8.md) | [Browser API](docs/browser-api-0.8-zh-TW.md) |
| [VS Code](vscode/README.md) | [VS Code](vscode/README-zh-TW.md) |

## Install / 安裝

### macOS / Linux

```bash
curl -fsSL https://raw.githubusercontent.com/funlearnstudio/SE/main/install.sh | sh
```

### Windows PowerShell

```powershell
irm https://raw.githubusercontent.com/funlearnstudio/SE/main/install.ps1 | iex
```

Verify / 確認：

```bash
se --version
se doctor
```

The prebuilt installer is enough for the REPL, `se run`, `se check`, `se check-all`, `se test`, and `se web build`. `se build` additionally requires a C++20 compiler because the native backend currently emits C++20.

預編譯安裝器即可使用 REPL、`se run`、`se check`、`se check-all`、`se test` 與 `se web build`。`se build` 目前會產生 C++20，因此另外需要 C++20 編譯器。

For source builds and platform details, see [Installation](docs/installation.md).

若要從原始碼編譯或查看各平台細節，請看 [Installation](docs/installation.md)。

## Core workflow / 基本工作流程

```bash
se check app.se       # static check / 靜態檢查
se run app.se         # interpreter / 直譯執行
se test .             # tests / 測試
se build app.se       # native executable / 原生執行檔
se web build app.se dist
```

Create projects / 建立專案：

```bash
se new app myapp
se new web mysite
```

## What SE includes / SE 包含什麼

- indentation-based blocks / 縮排區塊
- static checking and type inference / 靜態檢查與型別推斷
- functions, closures and generic functions / 函式、closure、泛型函式
- List, Map and Set
- user-defined types and methods / 自訂型別與方法
- modules / 模組
- recoverable errors / 可恢復錯誤
- Option and Result
- value-based `match` / `case`
- managed Task-style async / await
- file, path, time, math, random and OS utilities
- Bytes and JSON
- HTTP and HTTPS clients
- HTTP server and router
- lightweight persistent database APIs
- JavaScript and TypeScript bridges
- C ABI native interoperability and binding generation
- interpreter and C++20 native backend
- SE Web components, routing and browser API

The documentation separates **stable release behavior**, **newer current-source capabilities**, and **versioned design/roadmap work** so future-stage material is not confused with the released language.

文件會把**正式版本行為**、**目前 source 中較新的能力**與**版本化設計／Roadmap**分開，避免把未來階段內容誤當成正式 Release。

## Architecture / 架構

```text
SE source
    ↓
Lexer → Tokens + INDENT/DEDENT
    ↓
Pratt Parser
    ↓
AST
    ↓
Static Checker
    ├── Interpreter
    ├── C++20 Backend → native executable
    └── SE Web Compiler → HTML + CSS + JavaScript/TypeScript
```

## Compatibility / 相容性

SE is the successor name of the earlier S codebase. Internal C++ namespaces and some legacy `.s` compatibility may still use `s`, but new user-facing code should use **SE**, `se`, and `.se`.

SE 是早期 S codebase 的後繼名稱。內部 C++ namespace 與部分舊 `.s` 相容層仍可能保留 `s`，但新的使用者程式與文件應使用 **SE**、`se` 與 `.se`。

## License

MIT
