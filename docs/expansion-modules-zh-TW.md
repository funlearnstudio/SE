# SE 擴充模組

模組沿用 `use`、函式呼叫、List、Map、Text、Int、Num。錯誤輸入會回報 SE 錯誤；標成 fallible 的呼叫需要 `try`。以下是目前**已實作的函式**，同名模組不能推論為 Python 或其他語言中同名套件的完整功能。

| 模組 | 函式 | 說明與界限 |
| --- | --- | --- |
| `url` | `encode`, `decode`, `query`, `parse_query` | URL 百分比編碼、查詢參數；重複參數保留 Map 中的多個項目 |
| `encoding` | `hex`, `unhex`, `utf8_valid` | 位元組十六進位編解碼、UTF-8 驗證 |
| `dotenv`, `config` | `parse`, `get` | 簡單 `KEY=VALUE`；支援註解、`export` 和外層引號；無變數插值與多行值 |
| `array`, `series` | `sum`, `mean`, `slice` | 數值統計與左閉右開切片 |
| `matrix`, `linear` | `transpose`, `multiply`, `dot` | 等寬數值矩陣、矩陣乘積、向量內積 |
| `probability` | `factorial`, `choose` | 整數階乘上限 20；組合數 n 上限 66 |
| `fraction` | `make`, `decimal` | 約分後使用 `[分子, 分母]`，檢查零分母 |
| `complex` | `make`, `add`, `multiply`, `magnitude` | 使用 `[實部, 虛部]` |
| `calculus` | `polynomial`, `derivative`, `integral` | 係數依常數項起排列的一元多項式，積分為兩端點間定積分 |
| `units` | `convert`, `celsius_to_fahrenheit`, `fahrenheit_to_celsius` | 長度 m/km/cm/mm、時間 s/min/h、質量 kg/g/lb、溫度 |
| `table`, `dataset` | `column`, `row_count`, `select` | Map 列的欄位存取與投影 |
| `cookie` | `parse`, `set` | Cookie header 解析及預設 `HttpOnly; SameSite=Lax` 的 Set-Cookie 值 |
| `cors` | `allow_origin`, `preflight` | 建立 CORS 回應 header Map；會拒絕換行字元 |
| `toml` | `parse` | Python 3.11+ `tomllib`，回傳 SE 值；日期時間轉 ISO 文字 |
| `yaml` | `parse`, `stringify` | Python 3.11+ 加 PyYAML；使用 `safe_load` / `safe_dump` |
| `xml` | `parse`, `escape` | Python 3.11+ ElementTree；拒絕 DTD、entity 與超過 1 MB 的輸入 |
| `markdown` | `render` | Python 3.11+ 加 `markdown-it-py`，以 CommonMark 規則輸出 HTML |
| `crypto` | `sha256`, `hmac_sha256`, `random_hex`, `constant_time_equal` | Python 標準函式庫 hashlib/hmac/secrets |
| `jwt`, `session` | `sign`/`verify`, `encode`/`decode` | HS256 簽章，驗證時檢查 `exp`；session 為簽章資料而非伺服器儲存 |
| `auth` | `hash_password`, `verify_password` | PBKDF2-HMAC-SHA256、16 byte 隨機 salt、25 萬次迭代 |
| `http_server`, `router` | 與 `web` 相同 | 現有 `web` 執行器的 API：路由註冊、`listen`、`handle`、回應與請求資料。各 `use` 有獨立路由狀態 |
| `dns` | 與 `socket` 相同 | 現有 `socket.resolve` / `socket.tcp` 的別名 |
| `template` | `escape`, `render` | HTML 跳脫與 `{{key}}` 佔位符；缺值報錯，不執行程式碼 |
| `static` | `mime`, `read` | 常見 MIME 辨識、在指定根目錄內讀取檔案 |
| `upload` | `save` | 將文字／二進位 Text 寫進既有根目錄，拒絕跨目錄路徑 |
| `tilemap` | `parse`, `at`, `size` | 從等寬字元列讀取地圖、查詢格子與尺寸 |
| `gui`, `window`, `canvas`, `input`, `sprite`, `physics`, `sound`, `keyboard`, `mouse`, `animation`, `scene`, `collision`, `image`, `audio` | 與 `game` 相同 | 共享現有 game 場景的瀏覽器 HTML Canvas API；相機或本機裝置 API 未實作 |

範例見 [`examples/expansion.se`](../examples/expansion.se)、[`examples/expansion-math-data.se`](../examples/expansion-math-data.se)、[`examples/expansion-web.se`](../examples/expansion-web.se)、[`examples/expansion-content.se`](../examples/expansion-content.se)、[`examples/expansion-game.se`](../examples/expansion-game.se)、[`examples/expansion-formats.se`](../examples/expansion-formats.se)、[`examples/expansion-security.se`](../examples/expansion-security.se)。

尚未加入的名稱：`websocket`、`smtp`、`email`、`ai`、`ml`、`tensor`、`video`、`camera`、`ftp`、`imap`、`ssh` 等。現有 `game`、`net` 等模組已有部分相近功能。
