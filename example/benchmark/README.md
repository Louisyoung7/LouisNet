# LouisNet 压测工具

基于 wrk 的性能压测工具，用于评估 LouisNet 网络库在不同场景下的性能表现。

## 快速开始

```bash
# 1. 启动 HTTP 服务器
./build/release/example/benchmark/louis_net_benchmark

# 2. 运行指定场景压测（带优化标记）
./scenario_baseline.sh "优化标记"
./scenario_keepalive.sh "优化标记"

# 3. 查看结果
python3 benchmark_analyze.py -l
```

## 场景压测脚本

压测被拆分为多个独立的场景脚本，每个场景聚焦一个测试维度。所有脚本接受一个可选的**优化标记**参数，用于标识本次压测的优化内容。

### 场景一览

| 脚本 | 场景名称 | 测试重点 | 适用场景 |
|------|---------|---------|----------|
| [scenario_baseline.sh](scenario_baseline.sh) | 基线性能 | 标准 wrk 压测，覆盖低/中/高并发 | 每次优化后的必跑基准 |
| [scenario_keepalive.sh](scenario_keepalive.sh) | 大量空闲连接 | 高连接数低活跃度，ET vs LT 差异最明显 | 连接数敏感的优化对比 |
| [scenario_big_payload.sh](scenario_big_payload.sh) | 大包传输 | 大消息体吞吐，readv 和 ET 在此场景收益更明显 | 大消息体场景 |
| [scenario_pipeline.sh](scenario_pipeline.sh) | 管道测试 | 不同 pipeline 深度下延迟和吞吐变化 | 慢速客户端/管道场景 |
| [scenario_rampup.sh](scenario_rampup.sh) | 拐点测试 | 连接数逐步递增，观察 RPS 和 P99 拐点 | 找系统瓶颈连接数 |

### 使用方法

```bash
# 不带参数：默认标记为 unknown
./scenario_baseline.sh

# 带参数：记录本次压测的优化内容
./scenario_baseline.sh "LT→ET 模式切换"
./scenario_baseline.sh "优化Buffer对象池"
```

### 场景详情

#### 1. 基线测试 (scenario_baseline.sh)

标准 wrk 压测，覆盖低/中/高并发，作为优化前后的对比基准。

| 线程数 | 连接数 | 时长 | 场景说明 |
|--------|--------|------|----------|
| 1 | 100 | 10s | 基线：低并发单线程 |
| 2 | 500 | 30s | 中等并发 + 稳定性 |
| 2 | 1000 | 10s | 中等并发 |
| 4 | 2000 | 15s | 高并发多线程 |
| 8 | 4000 | 15s | 极限压力测试 |

#### 2. 大量空闲连接测试 (scenario_keepalive.sh)

模拟真实场景中大量长连接只有少量活跃的情况。**最能体现 ET vs LT 差异**的场景。

| 线程数 | 连接数 | 时长 | 场景说明 |
|--------|--------|------|----------|
| 2 | 5000 | 20s | 大量连接，轻度活跃 |
| 2 | 10000 | 20s | 更多连接，少量活跃 |
| 2 | 20000 | 15s | 极限空闲连接 |

#### 3. 大包传输测试 (scenario_big_payload.sh)

测试大消息体下的吞吐能力。包含两个子场景：
- **场景 A**：大响应体（需服务端支持 `/big` 端点）
- **场景 B**：HTTP Pipeline 深度测试（等效于大包传输）

#### 4. 管道测试 (scenario_pipeline.sh)

固定 2 线程/100 连接，测试不同 pipeline 深度下的表现。

| Pipeline 深度 | 说明 |
|---------------|------|
| 1 | 等价于普通 keep-alive 请求 |
| 5 / 10 / 20 / 50 / 100 | 深度越大，单次系统调用发送数据越多 |

#### 5. 拐点测试 (scenario_rampup.sh)

固定 2 线程，连接数从 100 逐步递增到 50000，观察 RPS 和 P99 延迟的拐点。

```bash
连接数: 100 → 500 → 1000 → 2000 → 5000 → 10000 → 20000 → 50000
```

### 输出说明

每次压测生成一个 JSON 文件到 `benchmark_results/` 目录，命名格式为 `{时间戳}_{场景名}.json`：

```json
{
  "timestamp": "2026-08-31_154639",
  "scenario": "baseline",
  "optimization": "LT→ET 模式切换",
  "git_commit": "a1b2c3d4",
  "git_branch": "main",
  "description": "标准 wrk 压测，覆盖低/中/高并发场景",
  "tests": [
    {
      "threads": 1,
      "connections": 100,
      "duration": 10,
      "rps": 364164.95,
      "avg_latency_us": 140.49,
      "p99_latency_us": 269.00,
      "timeout_count": 0,
      "transfer_per_sec": "5.40MB"
    }
  ]
}
```

### JSON 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `timestamp` | string | 压测时间 |
| `scenario` | string | 场景名称 |
| `optimization` | string | 优化标记/描述 |
| `git_commit` | string | Git commit hash |
| `tests[].threads` | int | 压测线程数 |
| `tests[].connections` | int | 压测连接数 |
| `tests[].duration` | int | 测试时长（秒） |
| `tests[].rps` | float | 每秒请求数 |
| `tests[].avg_latency_us` | float | 平均延迟（微秒） |
| `tests[].p99_latency_us` | float | P99 延迟（微秒） |
| `tests[].timeout_count` | int | 超时请求数 |
| `tests[].transfer_per_sec` | string | 传输速率 |

## 分析脚本 benchmark_analyze.py

### 依赖

```bash
pip install -r requirements.txt  # 如有
# 无外部依赖，纯标准库
```

### 使用方法

```bash
# 列出所有压测结果
python3 benchmark_analyze.py -l

# 对比两次压测（指定序号）
python3 benchmark_analyze.py -c 1 3

# 查看单次压测详情
python3 benchmark_analyze.py -d 2

# 导出所有结果到 CSV
python3 benchmark_analyze.py -e
```

### 命令行参数

| 参数 | 说明 |
|------|------|
| `-l, --list` | 列出所有历史压测结果 |
| `-c, --compare ID1 ID2` | 对比两次压测的性能差异 |
| `-d, --detail ID` | 显示单次压测的详细信息 |
| `-e, --export` | 导出所有结果到 CSV 文件 |

### 输出示例

**对比报告：**
```
======================================================================
压测结果对比
======================================================================
  旧: 2026-05-17_initial - baseline
  新: 2026-05-18_buffer_pool - 添加Buffer对象池
======================================================================

[4线程 / 2000连接]
  RPS:       77,408 → 95,000  (↑22.7%)
  平均延迟:  29,210μs → 22,500μs  (↓23.0%)
  P99延迟:     63,890μs → 48,000μs
  超时数:          0 → 0
======================================================================
```

## 结果解读

### 关键指标

| 指标 | 含义 | 理想值 |
|------|------|--------|
| **RPS** | 每秒请求数，越高越好 | 越高越好 |
| **平均延迟** | 请求平均响应时间，越低越好 | < 10ms |
| **P99 延迟** | 99% 请求的最大延迟 | < 100ms |
| **超时数** | 超时请求数量，应为 0 | 0 |

### 性能参考（虚拟机 4C8G，本地回环）

| 场景 | RPS | 评价 |
|------|-----|------|
| 1线程/100连接 | 15~18万 | 优秀 |
| 2线程/1000连接 | 10~14万 | 良好 |
| 4线程/2000连接 | 7~10万 | 一般 |
| 8线程/4000连接 | 4~6万 | 瓶颈 |

### 常见问题

**Q: 压测时出现大量 timeout？**
A: 连接数超过服务器处理能力，需要优化连接管理或增加资源。

**Q: 8线程比1线程还慢？**
A: 线程扩展性问题，检查是否正确使用了多 Reactor 架构。

**Q: RPS 波动大？**
A: 测试时长太短，建议至少 10s。系统资源争用也会影响稳定性。

**Q: 各个场景脚本如何选择？**
A: 建议每次优化后至少跑 `scenario_baseline.sh` 作为基准对比。如果涉及 epoll 模式切换（LT/ET），建议重点跑 `scenario_keepalive.sh` 观察差异。

## 文件结构

```
benchmark/
├── benchmark_common.sh          # 公共函数库（检查依赖、解析 wrk、JSON 输出）
├── scenario_baseline.sh         # 基线性能测试
├── scenario_keepalive.sh        # 大量空闲连接测试
├── scenario_big_payload.sh      # 大包传输测试
├── scenario_pipeline.sh         # 管道测试
├── scenario_rampup.sh           # 连接数递增拐点测试
├── benchmark_analyze.py         # 结果分析脚本
├── benchmark_results/           # 压测结果存放目录
│   ├── 2026-08-31_154639_baseline.json
│   ├── 2026-08-31_165613_keepalive.json
│   └── ...
└── README.md                    # 本文件
```

## 压测规范

1. **每次优化前**：跑一次 `scenario_baseline.sh` 作为基准
2. **每次优化后**：记录本次优化内容，带标记跑相关场景
3. **对比分析**：用 `-c` 对比优化前后的性能差异
4. **保留记录**：保留所有 JSON 结果，便于追溯

## 注意事项

- 压测前确保服务已启动并监听 `127.0.0.1:8080`
- 建议在测试环境运行，避免影响生产服务
- 压测结果是相对值，应在相同条件下对比
- 使用 `scenario_big_payload.sh` 场景 A 前，需确保服务端支持 `/big` 端点