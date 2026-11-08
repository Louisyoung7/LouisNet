# HTTP/1.1 完善计划

## 升级目标

1. **请求解析增强**：chunked transfer-encoding、方法/版本校验、query string 解析、header 加固
2. **方法级路由**：`registerHandler(method, path, cb)`，405/Allow 响应
3. **大小限制**（可配置）：请求行/header 行长度、header 数量、body 上限 → 413/431
4. **单元测试**（GTest）
5. 不做：超时控制、响应侧 chunked、HEAD/OPTIONS 特殊语义

## 关键设计决策

- **错误报告**：`parseRequest` 签名不变，新增成员 `Error error_` + `Error error() const`。`error_` 在**每次 `parseRequest` 入口清零**、`reset()` 不清它——因为出错路径内部会调 `reset()`，若一并清零 HttpServer 就读不到了
- **kError 语义（保留现状）**：清空整个缓冲 + 自动复位解析器。代价是管线化中错误请求后的合法请求被连带丢弃（`PipelinedFirstRequestInvalid` 已记录），服务端反正会关连接，可接受
- **成功路径统一自动复位（行为变更）**：现在仅 kBody 完成路径内部 reset，无 body 成功后状态停在 kHeaders（`ReuseBodylessWithoutResetIsError` 记录的怪癖）。重构后统一改为 `parseRequest` 包装层在 `kSuccess` 时调 `reset()`，`parseCString` 内部不再自行 reset。需同步把该测试改为"无 reset 复用也成功"。HttpServer 不受影响（`resetContext()` 本来就显式复位）
- **TE+CL 冲突**：按 RFC 7230 应优先 TE，但组合是请求走私经典向量，直接返回 400（`kTeClConflict`）最安全
- **方法校验**：只验证 RFC 7230 tchar 字符集（``!#$%&'*+-.^_`|~`` + 字母数字），不做已知方法白名单；未知合法方法由路由层 405 兜底
- **行解析重构**：所有行式状态统一用 `lineBuf_` 累积半行数据，跨 `kNeedMore` 可恢复，长度限制天然易实现
- **Content-Length 自动生成**：`toString()` 在 body 非空且未显式设置 CL 时自动输出，取代 `d382e3e` 的硬编码

## 实施步骤

### Step 1 — HttpRequest 增加 query 支持

`src/net/http/HttpRequest.h`：

- 新增成员 `std::string rawQuery_;`、`std::unordered_map<std::string, std::string> queryParams_;`，`reset()` 清空
- `path_` 从此只含 `?` 前段；现有测试均无 query，不受影响

### Step 2 — HttpParser 重构（核心）

`src/net/http/HttpParser.h`：

- 新增错误枚举与访问器：

```cpp
enum class Error {
    kNone, kMalformedRequestLine, kBadMethod, kBadVersion, kUnsupportedVersion,
    kBadHeader, kHeaderTooLarge, kRequestLineTooLarge, kTooManyHeaders,
    kBadContentLength, kTeClConflict, kBadTransferEncoding, kBodyTooLarge, kBadChunk
};
Error error() const { return error_; }
```

- 新增限制配置（成员默认值即可用，测试可改小）：

```cpp
struct Limits {
    size_t maxRequestLine = 8192;     // 请求行最大长度
    size_t maxHeaderLine = 8192;      // 单行 header 最大长度
    size_t maxHeaderCount = 100;      // header 最大条数
    size_t maxBodySize = 10 * 1024 * 1024;  // body 最大字节数（CL 与 chunked 总量共用）
    size_t maxChunkSizeLine = 1024;   // chunk-size 行最大长度
};
void setLimits(const Limits& limits) { limits_ = limits; }
```

- 状态机扩展：`kStartLine, kHeaders, kBody, kChunkSize, kChunkData, kChunkDataEnd, kTrailers`
- 新增成员：`Error error_; Limits limits_; std::string lineBuf_; size_t chunkRemaining_ = 0, chunkTotal_ = 0; int headerCount_ = 0;`

`src/net/http/HttpParser.cc`：

- **包装层调整**：`parseRequest` 入口 `error_ = Error::kNone`；`kSuccess` 时调 `reset()`；`kError` 时保持现有 `consumeAll() + reset()` 逻辑；`parseCString` 内部的 `reset()` 移除
- **行式解析统一模式**（kStartLine / kHeaders / kChunkSize / kTrailers 共用）：在 `cur` 中找 `"\r\n"`；找到则与 `lineBuf_` 拼接成完整行，校验长度限制后处理并清空 `lineBuf_`；找不到则把 `cur` 全部追加进 `lineBuf_`，检查限制后返回 `kNeedMore`。`parsed` 仍按 buffer 实际消费字节计
- **请求行校验**：恰好 2 个空格（连续空格视为非法）、方法/路径/版本均非空；方法逐字符验证 tchar → 否则 `kBadMethod`；target 按第一个 `?` 切出 path/rawQuery，query 解析（`&` 分割、首个 `=` 分名值、percent-decode、`+`→空格、非法 `%` 保留原样）→ `queryParams_`；版本须匹配 `HTTP/d.d`：1.0/1.1 通过、其他合法格式 → `kUnsupportedVersion`、格式错 → `kBadVersion`
- **Header 加固**：行首 SP/HTAB（obs-fold）→ `kBadHeader`；字段名内 SP/HTAB（走私向量）→ `kBadHeader`；字段名验证 tchar、非空；值允许为空；`headerCount_` 超 `maxHeaderCount` → `kTooManyHeaders`；**content-length**：多条必须值完全一致否则 `kBadContentLength`、逐字符验证纯数字（替换 `std::stoul`+catch，`InvalidContentLength` 测试已覆盖）且 ≤ `maxBodySize` 否则 `kBodyTooLarge`；**transfer-encoding**：取值（小写化后）最终编码须为 `chunked` 否则 `kBadTransferEncoding`，与 CL 并存 → `kTeClConflict`
- **chunked 状态机**（每次转移只消费已完整校验的字节）：
  - `kChunkSize`：`;` 前须有 ≥1 个 hex 位，`;` 后扩展忽略（无 CR/LF 控制字符即可）；size 超 `maxBodySize - chunkTotal_` → `kBodyTooLarge`；size>0 → `kChunkData`（`chunkRemaining_ = size`），size==0 → `kTrailers`
  - `kChunkData`：追加 `min(cur.size(), chunkRemaining_)` 到 `body_`，`chunkTotal_` 累计；收齐 → `kChunkDataEnd`
  - `kChunkDataEnd`：凑够 2 字节 `"\r\n"`（不足 2 字节时 `kNeedMore` 不消费）→ `kChunkSize`
  - `kTrailers`：每行计入 `maxHeaderCount`，空行 → `kSuccess`（trailer 本轮丢弃，不存入 headers_）
- `reset()` 清空：`lineBuf_`、`chunkRemaining_`、`chunkTotal_`、`headerCount_`、状态；**不清 `error_`**

### Step 3 — 方法级路由

`src/net/http/HttpServer.h/.cc`：

- `handlers_` 键改为 `"METHOD PATH"`（方法注册与查找时统一大写；任意方法注册键为 `"* PATH"`；path 不含空格，空格分隔无歧义）
- API：

```cpp
// 任意方法（保留旧接口语义）
void registerHandler(const std::string& path, RequestHandler cb);
// 指定方法
void registerHandler(const std::string& method, const std::string& path, RequestHandler cb);
```

- 抽出可单测的纯函数（不依赖 EventLoop/TcpServer）：

```cpp
enum class RouteResult { kFound, kMethodNotAllowed, kNotFound };
// outHandler 出参返回命中回调；allow 出参在 405 时填 "GET, POST" 形式
static RouteResult findRoute(const std::unordered_map<std::string, RequestHandler>& routes,
                             const std::string& method, const std::string& path,
                             const RequestHandler** outHandler, std::string* allow);
```

- 查找顺序：精确 `"METHOD PATH"` → `"* PATH"` → 该 path 存在其他方法键 → `kMethodNotAllowed`（allow 收集该 path 全部已注册方法）→ `kNotFound`

### Step 4 — 错误映射与响应修复

`src/net/http/HttpServer.cc`：

- `onMessage()` 的 `kError` 分支：按 `parser.error()` 映射状态码——`kBodyTooLarge`→413、`kHeaderTooLarge`/`kRequestLineTooLarge`/`kTooManyHeaders`→431、`kUnsupportedVersion`→505、其余→400；收敛为 `HttpResponse makeErrorResponse(int code, const char* text)` 辅助函数；**删除 `d382e3e` 加的硬编码 `setHeader("Content-Length", "11")`**（自动 CL 取代）
- `onRequest()`：处理三种 `RouteResult`——`kNotFound`→404、`kMethodNotAllowed`→405 + `Allow` 头；**删除硬编码 `setHeader("Content-Length", "9")`**
- 限制下发：新增 `void setHttpLimits(const HttpParser::Limits&)`，成员 `limits_`；`onConnection()` 构造 `HttpServerContext` 后调 `ctx->parser.setLimits(limits_)`

`src/net/http/HttpResponse.cc`：

- `getStatusMessage` 补充：405 "Method Not Allowed"、413 "Payload Too Large"、431 "Request Header Fields Too Large"、505 "HTTP Version Not Supported"
- `toString()`：body 非空且 headers_ 中无（**大小写不敏感比较**）`content-length` 键时，自动追加 `Content-Length: body_.size()`。已核对现有单测（`DefaultValues` 无 body 精确匹配、`find("Body")` 子串匹配）均兼容；`d382e3e` 的显式 CL 也会被大小写不敏感检测跳过
- 注意响应头键目前按传入大小写存储（代码中 "Connection"/"connection" 混用），本次仅做 CL 检测的大小写兼容，不统一改输出格式

### Step 5 — 单元测试

**扩展** `tests/http/test_http_parser.cc`（沿用其"边界/时序/生命周期/并发"分组风格，新增"chunked"与"限制"分组）：

- chunked 基础 / 带 chunk 扩展（`4;ext=1`）/ 带 trailer / 终止块后直接结束
- chunked 分片到达：size 行内、chunk 数据内、尾部 CRLF 内、trailer 内切断
- TE+CL 并存 → `error() == kTeClConflict`；TE 非 chunked → `kBadTransferEncoding`；重复 CL 相同→成功 / 不同→`kBadContentLength`；CL 超限 → `kBodyTooLarge`
- 非法方法 token（`GE(T`）→ `kBadMethod`；`HTTP/9.9` → `kUnsupportedVersion`；`HTTP/1.x` → `kBadVersion`
- obs-fold → `kBadHeader`；超长请求行/header 行/header 数量（改小 Limits）/ body 超限 / chunk size 行超限 → 对应错误
- 同一 parser 连续多个 chunked 请求（成功路径统一自动复位后的 reset 正确性）
- **修改既有测试** `ReuseBodylessWithoutResetIsError` → 改名为 `ReuseBodylessWithoutResetSucceeds`（成功路径统一自动复位后行为变更），并更新其注释

**新建** `tests/http/test_http_request.cc`：

- query 解析：`path_` 不含 `?`、`rawQuery_`、多参数、percent 解码（`%20`→空格、`%41`→A、`+`→空格）、无 `=` 参数、空 query（`/a?`）、多 `?`（首个 `?` 后全部视为 query）

**新建** `tests/http/test_http_router.cc`：

- `findRoute` 纯函数测试：精确命中、ANY（`*`）回退、405 + allow 字符串正确、404、方法大小写不敏感（注册小写查大写等）

路由端到端（真实 socket 405/Allow）留给手动 curl 验证（HttpServer 需要运行中的 EventLoop）。

### Step 6 — 示例更新（5 行内）

`example/http/main.cc`：注册一个方法级路由（如 `registerHandler("POST", "/echo", ...)`）演示 405/Allow。

## 验证

手动验证（从项目根目录）：

```bash
# chunked 请求体
curl -v --http1.1 -H "Transfer-Encoding: chunked" --data-binary "hello chunked world" http://127.0.0.1:8080/
# query + percent 解码
curl -v "http://127.0.0.1:8080/?name=a%20b&x=1"
# 405 + Allow
curl -v -X DELETE http://127.0.0.1:8080/echo
# 404（Content-Length 应为 9）
curl -v http://127.0.0.1:8080/nope
# 505 / 413：nc 手发 "GET /x HTTP/1.1x\r\n\r\n" 或超大 body
```

预期：chunked body 正确回显、query 正确解析、错误响应状态码与 CL 正确、错误后关闭连接。

## 关键文件

- `src/net/http/HttpParser.h/.cc`（核心重构）
- `src/net/http/HttpRequest.h`
- `src/net/http/HttpServer.h/.cc`
- `src/net/http/HttpResponse.cc`
- `tests/http/test_http_parser.cc`（扩展）、`tests/http/test_http_request.cc`（新建）、`tests/http/test_http_router.cc`（新建）
- `example/http/main.cc`
