# SE 快速開始

[English version](getting-started.md)

這份文件會帶你完成安裝、確認工具、執行第一支 SE 程式。現在的正式 Release 是 **SE 0.7.3**；如果你直接使用 GitHub `main` 原始碼，可能會包含更新的 compiler / Web 功能，但版本字串為 0.7。

## 1. 安裝 SE

### macOS / Linux

```bash
curl -fsSL https://raw.githubusercontent.com/funlearnstudio/SE/main/install.sh | sh
```

### Windows PowerShell

```powershell
irm https://raw.githubusercontent.com/funlearnstudio/SE/main/install.ps1 | iex
```

確認安裝：

```bash
se --version
se doctor
```

使用預編譯安裝版時，REPL、`run`、`check`、`check-all`、`test` 不需要 CMake 或 C++ compiler。

## 2. 從原始碼編譯 SE 本身

如果你要開發 SE compiler，或想使用 `main` 最新原始碼功能，使用這條路徑。

需求：

- Git
- CMake 3.20+
- C++20 compiler

```bash
git clone https://github.com/funlearnstudio/SE.git
cd SE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/se --version
```

之後 `git pull` 更新原始碼後，通常只需要重新：

```bash
cmake --build build --parallel
```

## 3. 第一支 SE 程式

建立 `hello.se`：

```se
say "Hello SE"
```

如果已安裝 SE：

```bash
se run hello.se
```

如果你是在 repository 裡使用尚未 install 的 build：

```bash
./build/se run hello.se
```

只做靜態檢查、不執行：

```bash
se check hello.se
```

## 4. 編譯成 Native Executable

```bash
se build hello.se
```

目前 Native Backend 會先產生 C++20，再呼叫系統 C++ compiler。因此即使 SE 本身是用預編譯套件安裝，執行 `se build` 仍需要 C++20 compiler。

## 5. REPL

```bash
se
```

例如：

```text
> say "Hello"
Hello
```

縮排 block 在 REPL 中用空白行結束。

## 6. 主要 CLI

```text
se
se --version
se help
se doctor
se check file.se
se check-all .
se run file.se
se test .
se build file.se
se new app myapp
se new web mysite
se web build app.se dist
se bind module.sbind generated
```

## 7. 建立專案

一般 App：

```bash
se new app myapp
cd myapp
se check-all .
se test .
se run src/main.se
```

Web 專案：

```bash
se new web mysite
cd mysite
```

Component Web source 可以執行：

```bash
se web build app.se dist
```

Web compiler 會輸出一般標準 Web 檔案，例如 `index.html`、`style.css`、`app.js`、`app.ts`。

## 8. 下一步

建議閱讀順序：

1. [完整教學](tutorial-zh-TW.md)
2. [語言參考](language-reference-zh-TW.md)
3. 想做網站：閱讀 [SE Web Language](web-language-0.8-zh-TW.md)
4. 想了解 compiler/runtime：閱讀 [技術參考](technical-reference-zh-TW.md)

新的 SE 原始碼應使用 `.se`。舊 `.s` 只保留在相容層或部分舊範例中。
