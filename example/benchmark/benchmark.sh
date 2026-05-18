#!/bin/bash

# wrk 梯度压测脚本 - 针对 C++ 网络库 (LouisNet)
# 目标：127.0.0.1:8080，虚拟机 4C8G，本地回环

# -------------------- 配置参数 --------------------
HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}/"

# 确保服务已启动并监听
echo "检查服务是否在 ${HOST}:${PORT} 上监听..."
if ! ss -lnt | grep -q ":${PORT} "; then
    echo "错误：未检测到监听 ${PORT} 的服务，请先启动你的网络库服务端程序。"
    exit 1
fi

# 创建结果目录
RESULT_DIR="wrk_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"

# 汇总文件
SUMMARY_FILE="${RESULT_DIR}/summary.txt"

# 输出颜色
GREEN='\033[0;32m'
NC='\033[0m'

# -------------------- 梯度定义 --------------------
# 每组测试: 线程数 连接数 持续时间(秒)
# 注意：本地回环且 4C8G，连接数和线程数不宜过高

# 从低压力到高压力
BENCHMARKS=(
    "1 100 10"
    "1 500 10"
    "2 500 10"
    "2 1000 10"
    "2 2000 15"
    "4 1000 15"
    "4 2000 15"
)

# -------------------- 开始测试 --------------------
echo -e "${GREEN}开始梯度压测（终端输出）${NC}"
echo "压测时间: $(date)"
echo "目标 URL: $URL"
echo "虚拟机配置: 4C8G, 本地回环"
echo "----------------------------------------------"

for bench in "${BENCHMARKS[@]}"; do
    read -r threads connections duration <<< "$bench"

    echo -e "${GREEN}[测试] 线程=${threads} 连接=${connections} 时长=${duration}s${NC}"

    RESULT_FILE="${RESULT_DIR}/result_t${threads}_c${connections}.txt"

    wrk --latency -d"${duration}" -t"${threads}" -c"${connections}" "$URL" | tee "$RESULT_FILE"

    {
        echo "线程=${threads} 连接=${connections} 时长=${duration}s"
        grep -E "Requests/sec|Transfer/sec|Latency" "$RESULT_FILE" 2>/dev/null || echo "  (详情见 result 文件)"
        echo "---"
    } >> "$SUMMARY_FILE"

    sleep 2
done

echo -e "${GREEN}压测完成！结果保存在: ${RESULT_DIR}/${NC}"