#!/bin/bash

# wrk 压测脚本 - 针对 C++ 网络库 (LouisNet)
# 目标：127.0.0.1:8080，虚拟机 4C8G，本地回环

# -------------------- 配置参数 --------------------
HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

# 结果目录（统一存放所有压测结果）
RESULT_DIR="benchmark_results"
mkdir -p "$RESULT_DIR"

# -------------------- 检查服务 --------------------
echo "检查服务是否在 ${HOST}:${PORT} 上监听..."
if ! ss -lnt | grep -q ":${PORT} "; then
    echo "错误：未检测到监听 ${PORT} 的服务，请先启动你的网络库服务端程序。"
    exit 1
fi

# -------------------- 收集元数据 --------------------
TIMESTAMP=$(date +"%Y-%m-%d_%H%M%S")
OPTIMIZATION="${1:-baseline}"

# 获取 git 信息（如果没有 git 仓库则为空）
GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "")

# 输出颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# -------------------- 测试用例 --------------------
# 线程数 连接数 持续时间(秒)
BENCHMARKS=(
    "1 100 10"      # 基线：低并发
    "2 500 60"      # 稳定性测试：中等并发 + 长时间
    "2 1000 10"     # 中等并发
    "4 2000 15"     # 高并发
    "8 4000 15"     # 极限压力
)

# -------------------- 初始化 JSON --------------------
JSON_FILE="${RESULT_DIR}/${TIMESTAMP}.json"
> "$JSON_FILE"

# 写入 JSON 头部
cat > "$JSON_FILE" << EOF
{
  "timestamp": "${TIMESTAMP}",
  "optimization": "${OPTIMIZATION}",
  "git_commit": "${GIT_COMMIT}",
  "git_branch": "${GIT_BRANCH}",
  "url": "${URL}",
  "tests": [
EOF

# -------------------- 开始测试 --------------------
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}开始压测 - ${TIMESTAMP}${NC}"
echo -e "${GREEN}优化标记: ${OPTIMIZATION}${NC}"
echo -e "${GREEN}========================================${NC}"

FIRST=true
for bench in "${BENCHMARKS[@]}"; do
    read -r threads connections duration <<< "$bench"

    echo -e "\n${GREEN}[${threads}线程/${connections}连接/${duration}s]${NC}"

    # 执行压测
    RESULT=$(wrk --latency -d"${duration}" -t"${threads}" -c"${connections}" "$URL" 2>&1)

    # 解析结果（wrk 输出格式）
    # Latency 行: "    Latency   377.82us  347.97us  12.27ms   93.95%"
    AVG_LAT=$(echo "$RESULT" | grep "Latency$" | awk '{print $2}' | sed 's/[^0-9.]//g')
    # P99 行: "     99%    1.63ms"
    P99_LAT=$(echo "$RESULT" | grep "99%" | awk '{print $2}' | sed 's/[^0-9.]//g')
    # timeout 行: "  Socket errors: connect 0, read 0, write 0, timeout 342"
    TIMEOUT=$(echo "$RESULT" | grep "timeout" | sed 's/.*timeout *\([0-9]*\).*/\1/')
    # RPS 行: "Requests/sec: 173188.32"
    RPS=$(echo "$RESULT" | grep "Requests/sec" | awk '{print $2}' | cut -d'.' -f1)
    # Transfer 行: "Transfer/sec:     14.53MB"
    TRANSFER=$(echo "$RESULT" | grep "Transfer/sec" | awk '{print $2}')

    # 默认值处理
    P99_LAT="${P99_LAT:-0}"
    TIMEOUT="${TIMEOUT:-0}"
    AVG_LAT="${AVG_LAT:-0}"

    echo "  RPS: $RPS | 平均延迟: $AVG_LAT | P99: $P99_LAT | 超时: $TIMEOUT"

    # 追加到 JSON（格式化输出）
    if [ "$FIRST" = true ]; then
        FIRST=false
    else
        echo "," >> "$JSON_FILE"
    fi

    cat >> "$JSON_FILE" << EOF
    {
      "threads": ${threads},
      "connections": ${connections},
      "duration": ${duration},
      "rps": ${RPS:-0},
      "avg_latency_us": ${AVG_LAT:-0},
      "p99_latency_us": ${P99_LAT:-0},
      "timeout_count": ${TIMEOUT:-0},
      "transfer_mb_per_sec": "${TRANSFER:-0MB}"
    }
EOF

    sleep 2
done

# 写入 JSON 尾部
cat >> "$JSON_FILE" << EOF

  ]
}
EOF

echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}压测完成！${NC}"
echo -e "结果文件: ${JSON_FILE}"
echo -e "${GREEN}========================================${NC}"