# LouisNet 压测工具

基于 wrk 的性能压测工具，用于评估 LouisNet 网络库在不同并发场景下的性能表现。

## 快速开始

```bash
# 1. 启动 HTTP 服务器
./build/release/example/benchmark/louis_net_benchmark

# 2. 运行压测（带优化标记）
./benchmark.sh "优化Buffer对象池"

# 3. 查看结果
python3 benchmark_analyze.py -l
```

## 压测脚本 benchmark.sh

### 使用方法

```bash
./benchmark.sh [优化标记]
```

- 不带参数：默认标记为 `baseline`
- 带参数：记录本次压测的优化内容

```bash
./benchmark.sh                      # 初始基准测试
./benchmark.sh "添加Buffer对象池"    # 优化后的测试
./benchmark.sh "修复连接泄漏"        # 又一次优化
```

### 测试用例

| 序号 | 线程数 | 连接数 | 时长 | 场景说明 |
|------|--------|--------|------|----------|
| 1 | 1 | 100 | 10s | 基线：低并发单线程 |
| 2 | 2 | 500 | 60s | **稳定性测试**：中等并发 + 长时间，验证内存泄漏和性能稳定性 |
| 3 | 2 | 1000 | 10s | 中等并发 |
| 4 | 4 | 2000 | 15s | 高并发多线程 |
| 5 | 8 | 4000 | 15s | 极限压力测试 |

> **稳定性测试说明**：60 秒持续压测用于检测连接泄漏、内存泄漏、连接超时等隐藏问题。建议每次重大优化后必跑。

### 输出说明

每次压测生成一个 JSON 文件到 `benchmark_results/` 目录：

```json
{
  "timestamp": "2026-05-17_13_00_00",
  "optimization": "添加Buffer对象池",
  "git_commit": "a1b2c3d4",
  "git_branch": "main",
  "tests": [
    {
      "threads": 1,
      "connections": 100,
      "rps": 180000,
      "avg_latency_us": 377,
      "p99_latency_us": 1630,
      "timeout_count": 0,
      "transfer_mb_per_sec": "14.53MB"
    }
  ]
}
```

### JSON 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `timestamp` | string | 压测时间 |
| `optimization` | string | 优化标记/描述 |
| `git_commit` | string | Git commit hash |
| `tests[].threads` | int | 压测线程数 |
| `tests[].connections` | int | 压测连接数 |
| `tests[].rps` | int | 每秒请求数 |
| `tests[].avg_latency_us` | int | 平均延迟（微秒） |
| `tests[].p99_latency_us` | int | P99 延迟（微秒） |
| `tests[].timeout_count` | int | 超时请求数 |
| `tests[].transfer_mb_per_sec` | string | 传输速率 |

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

## 文件结构

```
benchmark/
├── benchmark.sh              # 压测执行脚本
├── benchmark_analyze.py     # 结果分析脚本
└── benchmark_results/       # 压测结果存放目录
    ├── 2026-05-17_initial.json
    ├── 2026-05-18_buffer_pool.json
    └── all_results.csv       # CSV 导出（可选）
```

## 压测规范

1. **每次优化前**：先跑一次 baseline
2. **每次优化后**：记录本次优化内容，带标记跑压测
3. **对比分析**：用 `-c` 对比优化前后的性能差异
4. **保留记录**：保留所有 JSON 结果，便于追溯

## 注意事项

- 压测前确保服务已启动并监听 `127.0.0.1:8080`
- 建议在测试环境运行，避免影响生产服务
- 压测结果是相对值，应在相同条件下对比
