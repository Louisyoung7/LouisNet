#!/bin/bash

# wrk 压测脚本 - 针对 C++ 网络库 (LouisNet)
# 目标：127.0.0.1:8080，虚拟机 8C8G，本地回环

# ==================== 配置参数 ====================
HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

RESULT_DIR="benchmark_results"
mkdir -p "$RESULT_DIR"

# ==================== 检查依赖 ====================
for cmd in wrk ss awk; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "错误：未找到命令 '$cmd'，请先安装。"
        exit 1
    fi
done

# ==================== 检查服务 ====================
echo "检查服务是否在 ${HOST}:${PORT} 上监听..."
if ! ss -lnt | grep -q ":${PORT} "; then
    echo "错误：未检测到监听 ${PORT} 的服务，请先启动你的网络库服务端程序。"
    exit 1
fi

# 额外验证：实际发一个请求，确认可达
if ! curl -sf --connect-timeout 3 -o /dev/null "$URL" 2>/dev/null; then
    echo "警告：端口在监听但请求失败，服务可能未完全就绪，仍继续测试。"
    sleep 2
fi

# ==================== 收集元数据 ====================
TIMESTAMP=$(date +"%Y-%m-%d_%H%M%S")
OPTIMIZATION="${1:-baseline}"

GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")

# 输出颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

# ==================== 辅助函数 ====================

# 将带单位的延迟值统一转为微秒 (us)
# 输入: "377.82us" / "1.23ms" / "2.5s" / ""
# 输出: 377.82 / 1230.00 / 2500000.00 / 0
to_us() {
    local raw="$1"
    if [[ -z "$raw" ]]; then
        echo "0"
        return
    fi

    local num unit
    num=$(echo "$raw" | grep -oE '[0-9]+\.?[0-9]*' || echo "0")
    unit=$(echo "$raw" | grep -oE '[a-zA-Zμ]+' || echo "us")
    unit=$(echo "$unit" | tr '[:upper:]' '[:lower:]')

    case "$unit" in
        us|μs) echo "$num" ;;
        ms)    awk "BEGIN {printf \"%.2f\", $num * 1000}" ;;
        s|sec) awk "BEGIN {printf \"%.2f\", $num * 1000000}" ;;
        *)     echo "$num" ;;
    esac
}

# ==================== 测试用例 ====================
# 线程数 连接数 持续时间(秒)
BENCHMARKS=(
    "1 100 10"      # 基线：低并发
    "2 500 60"      # 稳定性测试：中等并发 + 长时间
    "2 1000 10"     # 中等并发
    "4 2000 15"     # 高并发
    "8 4000 15"     # 极限压力
)

# ==================== 初始化 JSON ====================
JSON_FILE="${RESULT_DIR}/${TIMESTAMP}.json"

cat > "$JSON_FILE" << EOF
{
  "timestamp": "${TIMESTAMP}",
  "optimization": "${OPTIMIZATION}",
  "git_commit": "${GIT_COMMIT}",
  "git_branch": "${GIT_BRANCH}",
  "url": "${URL}",
  "tests": [
EOF

# ==================== 开始测试 ====================
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}开始压测 - ${TIMESTAMP}${NC}"
echo -e "${GREEN}优化标记: ${OPTIMIZATION}${NC}"
echo -e "${GREEN}========================================${NC}"

FIRST=true
for bench in "${BENCHMARKS[@]}"; do
    read -r threads connections duration <<< "$bench"

    echo -e "\n${GREEN}[${threads}线程 / ${connections}连接 / ${duration}s]${NC}"

    # 执行压测（|| true 防止 wrk 非零退出导致脚本中断）
    RESULT=$(wrk --latency -d"${duration}" -t"${threads}" -c"${connections}" "$URL" 2>&1) || true

    # ---- 解析结果 ----

    # [修复] Avg Latency
    # 原: grep "Latency$" → 永远匹配不到（"Latency" 后面还有数据，不是行尾）
    # 改: 匹配行首空格 + "Latency" + 空格，取第2列
    RAW_AVG=$(echo "$RESULT" | awk '/^[[:space:]]+Latency[[:space:]]/ {print $2}')
    AVG_LAT=$(to_us "$RAW_AVG")

    # [修复] P99 Latency
    # 原: grep "99%" 可能误匹配 "99.9%" 等行
    # 改: 精确匹配 "99%" 且前面是空格
    RAW_P99=$(echo "$RESULT" | awk '/^[[:space:]]+99%/ {print $2}')
    P99_LAT=$(to_us "$RAW_P99")

    # [修复] Timeout
    # 原: 无 timeout 行时 sed 对空输入行为不可靠
    # 改: grep -oP 提取，无匹配时默认 0
    TIMEOUT=$(echo "$RESULT" | grep -oP 'timeout\s+\K[0-9]+' || echo "0")

    # [修复] RPS
    # 原: cut -d'.' -f1 丢弃小数部分
    # 改: 保留完整数值
    RPS=$(echo "$RESULT" | grep -oP 'Requests/sec:\s*\K[0-9]+\.?[0-9]*' || echo "0")

    # [修复] Transfer
    # 原: 只取数值，丢失单位
    # 改: 保留 "5.40MB" 完整字符串，字段名改为 transfer_per_sec
    TRANSFER=$(echo "$RESULT" | grep -oP 'Transfer/sec:\s*\K\S+' || echo "0B")

    # 默认值兜底
    AVG_LAT="${AVG_LAT:-0}"
    P99_LAT="${P99_LAT:-0}"
    TIMEOUT="${TIMEOUT:-0}"
    RPS="${RPS:-0}"

    echo "  RPS: $RPS | 平均延迟: ${AVG_LAT}μs | P99: ${P99_LAT}μs | 超时: $TIMEOUT"

    # 追加到 JSON
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
      "rps": ${RPS},
      "avg_latency_us": ${AVG_LAT},
      "p99_latency_us": ${P99_LAT},
      "timeout_count": ${TIMEOUT},
      "transfer_per_sec": "${TRANSFER}"
    }
EOF

    # [改进] 高并发后多等几秒，让内核回收 TIME_WAIT
    sleep 3
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