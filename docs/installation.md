# SE Installation / SE 安裝

Current stable release / 目前正式版：**SE 0.7.0**

SE provides prebuilt packages for normal use. Git, CMake and a C++ compiler are only required when building SE itself from source. `se build` separately requires a C++20 compiler because the native backend currently emits C++20.

SE 提供預編譯套件。一般使用不需要 Git、CMake 或 C++ compiler；只有從原始碼編譯 SE 本身時才需要。`se build` 另外需要 C++20 compiler，因為目前 Native Backend 會產生 C++20。

## Quick install / 快速安裝

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

## What works without a compiler? / 哪些功能不需要 compiler？

```bash
se
se run hello.se
se check hello.se
se check-all .
se test .
se new app myapp
se new web mysite
se web build app.se dist
```

These use the installed SE executable. / 這些功能直接使用已安裝的 SE executable。

## Native executable / Native 編譯

```bash
se build hello.se
```

This command requires a system C++20 compiler. CMake is not required merely to run an installed SE package.

這個指令需要系統 C++20 compiler。單純執行已安裝的 SE 不需要 CMake。

## Build SE from source / 從原始碼編譯 SE

Requirements / 需求：

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

After pulling new source / `git pull` 更新後：

```bash
cmake --build build --parallel
```

If an example exists on GitHub `main` but not in your local checkout, update your repository before assuming the compiler is missing the feature:

如果 GitHub `main` 有某個 example，但你的本機找不到，先同步 repository，不要直接判斷 compiler 沒有功能：

```bash
git switch main
git pull --ff-only
```

## PATH troubleshooting / PATH 問題

macOS / Linux:

```bash
which se
se --version
```

If needed / 如有需要：

```bash
export PATH="$HOME/.local/bin:$PATH"
```

To keep it in zsh / 永久加入 zsh：

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Windows PowerShell:

```powershell
Get-Command se
se --version
```

Open a new terminal after installation if PATH changes are not visible yet.

如果安裝後目前 Terminal 還看不到新的 PATH，重新開啟 Terminal。

## HTTPS note / HTTPS 注意事項

The current SE HTTPS module uses system `curl`, so HTTPS functionality requires `curl`.

目前 SE HTTPS module 使用系統 `curl`，因此 HTTPS 功能需要 `curl`。

## Next / 下一步

- [Getting Started](getting-started.md) / [快速開始](getting-started-zh-TW.md)
- [Tutorial](tutorial.md) / [完整教學](tutorial-zh-TW.md)
- [Documentation index](README.md) / [文件總覽](README-zh-TW.md)
