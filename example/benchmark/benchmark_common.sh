#!/bin/bash
#
# benchmark_common.sh - 公共函数库
# 被各 scenario_*.sh 脚本 source 使用
# ==============================================

# ==================== 配置 ====================
HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"
RESULT_DIR="benchmark_results"
mkdir -p "$RESULT_DIR"

# 颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ==================== 检查依赖 ====================
check_deps() {
    local missing=0
    for cmd in wrk awk; do
        if ! command -v "$cmd" &>/dev/null; then
            echo "错误：未找到命令 '$cmd'，请先安装。"
            missing=1
        fi
    done
    if [ "$missing" -eq 1 ]; then
        exit 1
    fi
}

# ==================== 检查服务 ====================
check_service() {
    echo "检查服务是否在 ${HOST}:${PORT} 上监听..."
    if ! ss -lnt 2>/dev/null | grep -q ":${PORT} "; then
        echo "错误：未检测到监听 ${PORT} 的服务，请先启动服务端程序。"
        exit 1
    fi
    # 额外验证
    if ! curl -sf --connect-timeout 3 -o /dev/null "$URL" 2>/dev/null; then
        echo "警告：端口在监听但请求失败，服务可能未完全就绪，仍继续测试。"
        sleep 2
    fi
}

# ==================== 收集元数据 ====================
init_metadata() {
    TIMESTAMP=$(date +"%Y-%m-%d_%H%M%S")
    SCENARIO="$1"
    OPTIMIZATION="${2:-unknown}"
    GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
    GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
}

# ==================== 延迟单位转换 ====================
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

# ==================== 执行单次压测并返回 JSON 片段 ====================
# 参数: 线程数 连接数 持续时间(秒) [额外wrk参数...]
run_test() {
    local threads="$1"
    local connections="$2"
    local duration="$3"
    shift 3
    local extra_args="$@"

    echo -e "  [${threads}线程 / ${connections}连接 / ${duration}s]"

    local RESULT
    RESULT=$(wrk --latency -d"${duration}" -t"${threads}" -c"${connections}" $extra_args "$URL" 2>&1) || true

    # 解析结果
    RAW_AVG=$(echo "$RESULT" | awk '/^[[:space:]]+Latency[[:space:]]/ {print $2}')
    AVG_LAT=$(to_us "$RAW_AVG")
    RAW_P99=$(echo "$RESULT" | awk '/^[[:space:]]+99%/ {print $2}')
    P99_LAT=$(to_us "$RAW_P99")
    TIMEOUT=$(echo "$RESULT" | grep -oP 'timeout\s+\K[0-9]+' || echo "0")
    RPS=$(echo "$RESULT" | grep -oP 'Requests/sec:\s*\K[0-9]+\.?[0-9]*' || echo "0")
    TRANSFER=$(echo "$RESULT" | grep -oP 'Transfer/sec:\s*\K\S+' || echo "0B")

    AVG_LAT="${AVG_LAT:-0}"
    P99_LAT="${P99_LAT:-0}"
    TIMEOUT="${TIMEOUT:-0}"
    RPS="${RPS:-0}"

    echo "    RPS: $RPS | 平均延迟: ${AVG_LAT}μs | P99: ${P99_LAT}μs | 超时: $TIMEOUT"

    # 生成 JSON 片段
    cat << EOF
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
}

# ==================== 初始化 JSON 文件 ====================
# 参数: 场景名 优化标记
init_json() {
    JSON_FILE="${RESULT_DIR}/${TIMESTAMP}_${1}.json"

    cat > "$JSON_FILE" << EOF
{
  "timestamp": "${TIMESTAMP}",
  "scenario": "${1}",
  "optimization": "${OPTIMIZATION}",
  "git_commit": "${GIT_COMMIT}",
  "git_branch": "${GIT_BRANCH}",
  "url": "${URL}",
  "description": "${2}",
  "tests": [
EOF
    echo "$JSON_FILE"
}

# ==================== 结束 JSON 文件 ====================
# 参数: JSON_FILE 路径
finish_json() {
    local file="$1"
    cat >> "$file" << EOF

  ]
}
EOF
    echo -e "\n${GREEN}结果文件: ${file}${NC}"
}

# ==================== 打印场景标题 ====================
print_scene_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}场景: ${1}${NC}"
    echo -e "${BLUE}说明: ${2}${NC}"
    echo -e "${BLUE}优化标记: ${OPTIMIZATION}${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# ==================== 测试间等待 ====================
test_pause() {
    sleep 2
}