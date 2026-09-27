# SE 擴充模組（第一批）

這批模組使用現有的 `use` 語法與 SE 的 List、Map、Text、Int、Num。需要 `try` 的函式會對錯誤輸入給出明確錯誤。

| 模組 | 可用函式 | 範圍 |
| --- | --- | --- |
| `url` | `encode(text)`, `decode(text)`, `query(map)`, `parse_query(text)` | 百分比編碼與查詢字串；`parse_query` 遇到重複鍵會保留多個 Map 項目 |
| `encoding` | `hex(text)`, `unhex(text)`, `utf8_valid(text)` | UTF-8 字串檢查、位元組十六進位編解碼 |
| `dotenv`, `config` | `parse(text)`, `get(map,key)` | 簡單 `KEY=VALUE` 文字；支援註解、`export`、外層引號，不解析插值或多行引號 |
| `array`, `series` | `sum(list)`, `mean(list)`, `slice(list,start,end)` | 數值總和、平均、左閉右開的索引切片 |
| `matrix`, `linear` | `transpose(rows)`, `multiply(a,b)`, `dot(a,b)` | 等寬數值矩陣、矩陣乘積與向量內積 |
| `probability` | `factorial(n)`, `choose(n,k)` | 整數階乘及組合數；階乘最大 20、組合數 n 最大 66 |

`dotenv` 和 `config`、`array` 和 `series`、`matrix` 和 `linear` 目前各是一組相同 API 的別名。

```se
use matrix
use url

product = try matrix.multiply [[1, 2]] [[3], [4]]
say product
say url.encode "SE & friends"
```

這一批尚未實作清單中的 `crypto`、網路伺服器、郵件、完整資料格式解析器、影像影音裝置、GUI、機器學習、網頁 session 等模組。已有的 `net`、`game`、`statistics` 等模組請參閱既有文件。後續模組要分別確認底層依賴、安全邊界、跨平台行為與測試後再加入。
