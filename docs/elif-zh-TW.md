# SE `else if` / `elif`

SE 的條件分支同時支援完整寫法 `else if` 與簡寫 `elif`。兩者語意完全相同，可以依照程式風格選擇。

```se
score = 82

if score >= 90
    say "A"
elif score >= 80
    say "B"
elif score >= 70
    say "C"
else
    say "D"
```

也可以寫成：

```se
if score >= 90
    say "A"
else if score >= 80
    say "B"
else if score >= 70
    say "C"
else
    say "D"
```

`elif` 在語言前端會降低成既有的 `else if` 結構，因此 Interpreter、Type Checker、Native C++ Backend 與 SE Web 共用相同的 If AST 與執行語意。
