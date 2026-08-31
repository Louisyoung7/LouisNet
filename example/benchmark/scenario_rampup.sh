#!/bin/bash
#
# scenario_rampup.sh - 连接数递增，寻找性能拐点
# 用途: 通过逐步增加连接数，观察 RPS 和延迟的变化曲线
# 特点: 固定线程数，连接数从 100 逐步增加到 50000，找到拐点
# ==============================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./benchmark_common.sh

OPTIMIZATION="${1:-unknown}"

check_deps
check_service
init_metadata "rampup" "$OPTIMIZATION"

print_scene_header "连接数递增拐点测试" "固定线程数，逐步增加连接数，观察 RPS 和 P99 延迟的拐点"

JSON_FILE=$(init_json "rampup" "固定线程数，逐步增加连接数，观察 RPS 和 P99 延迟的拐点")

# 固定 2 线程，持续 15s
# 连接数从 100 → 50000 逐步递增
THREADS=2
DURATION=15

CONNECTIONS=(
    100
    500
    1000
    2000
    5000
    10000
    20000
    50000
)

echo -e "  ${YELLOW}固定: ${THREADS}线程, ${DURATION}s/轮${NC}"
echo -e "  ${YELLOW}连接数: ${CONNECTIONS[*]}${NC}"

FIRST=true
for conn in "${CONNECTIONS[@]}"; do
    JSON_FRAG=$(run_test "$THREADS" "$conn" "$DURATION")

    if [ "$FIRST" = true ]; then
        FIRST=false
        echo -n "$JSON_FRAG" >> "$JSON_FILE"
    else
        echo "," >> "$JSON_FILE"
        echo -n "$JSON_FRAG" >> "$JSON_FILE"
    fi

    # 连接数越大，等待越久让 TIME_WAIT 回收
    if [ "$conn" -gt 10000 ]; then
        sleep 5
    else
        test_pause
    fi

done

finish_json "$JSON_FILE"
echo -e "\n${GREEN}拐点测试压测完成！${NC}"