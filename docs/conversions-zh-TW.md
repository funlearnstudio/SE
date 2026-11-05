# SE 核心型別轉換

[English version](conversions.md)

SE 提供一組不需要 `use` 的核心 conversion helper。這些 helper 會轉成既有 SE runtime type，不會另外建立重複的 `String`、`Double`、`Float` 或 `Char` 型別系統。

## Text

```se
say text 123
say string 123
```

兩者都回傳 `Text`。

## Integer

```se
age = int "15"
count = integer "42"
```

`int` / `integer` 都回傳 `Int`。無法合法轉成整數時會產生 conversion error，而不是默默回傳 0。

## Number

```se
price = num "19.95"
a = double "3.14"
b = float 2
```

`num`、`double`、`float` 都回傳 SE `Num`。

## Boolean

```se
ready = bool "true"
enabled = boolean "FALSE"
```

`bool` / `boolean` 都回傳 `Bool`。Text conversion 會接受 runtime 支援的 true/false 大小寫形式。

## Character Helper

SE 目前沒有獨立 `Char` runtime type。`char` 回傳「剛好一個 Unicode 字元」的 `Text`。

```se
letter = char "A"
han = char "中"
letter2 = char 65
```

空字串或多字元輸入會被拒絕。

## 搭配 ask

原有的 `ask "問題"` 仍然讀取整行並回傳 Text：

```se
number = int ask "Enter a number: "
```

### 一行輸入多個值（新增）

在同一行指定至少兩個變數，並用空格分隔輸入：

```se
ask a:Int b:Int
say a + b
```

輸入 `10 20`，將輸出 `30`。

支援 `Int`（整數）、`Num`（浮點數）和 `Text`（文字）。未標型別的變數預設為 Text：

```se
ask age:Int height:Num name:Text
# 輸入：16 170.5 Steve
say age
say height
say name

ask first second
# 輸入：Hello SE
say first + second
```

**規則：** `ask` 讀取一行並以空白字元分割；輸入欄位數須與變數數量完全相符。不能把含空格的片語當成單一 Text 值。數字格式錯誤會回報 SE Error，且不會先寫入部分變數。舊式 `name = ask "問題"` 不受影響。

## Alias Table

| Helper | 結果 |
| --- | --- |
| `text value` | `Text` |
| `string value` | `Text` |
| `int value` | `Int` |
| `integer value` | `Int` |
| `num value` | `Num` |
| `double value` | `Num` |
| `float value` | `Num` |
| `bool value` | `Bool` |
| `boolean value` | `Bool` |
| `char value` | 單一 Unicode 字元的 `Text` |
