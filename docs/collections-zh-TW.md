# SE Collections

[English version](collections.md)

SE 提供 managed `List`、`Map`、`Set`。Collection operation 由 runtime 做安全檢查，不把 C++ iterator 或 raw memory 暴露給一般 SE code。

## List

```se
nums = [1, 2, 3]
nums.add 4
nums.remove 2
say nums.len
say nums[0]
```

Index 超出範圍時會產生 SE error，而不是去讀未知記憶體。

## Map

```se
user = ["name": "SE", "score": 100]
say user["name"]
removed = user.remove "score"   # Bool：成功刪除回傳 true
```

Map 很適合搭配 JSON-style data。

遍歷 key/value：

```se
for key value in user
    say key
    say value
```

## Set

```se
values = set [1, 2, 2, 3]
values.add 4
values.remove 2

if 3 in values
    say "found"
```

Set 只保留 unique value。

## Higher-order Collection Helpers

```se
use collections

filtered = collections.filter nums predicate
mapped = collections.map nums transform
sum = collections.reduce nums 0 add
part = collections.slice nums 1 3
first = collections.take nums 2
rest = collections.drop nums 2
```

排序：

```se
ordered = collections.sort_by users "score"
desc = collections.sort_by_desc users "score"
ordered2 = collections.sort_with nums before
```

這些 helper 原則上回傳新的 List，不透過 C++ container internals 暴露 mutation 細節。

## Type Information

Checker 能推斷時會保留 collection element/value type。Heterogeneous JSON 或 dynamic platform data 在無法精確靜態判斷時，可能得到較寬鬆／Unknown 型別。
