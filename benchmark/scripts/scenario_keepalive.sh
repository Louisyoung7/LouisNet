#!/bin/bash
#
# scenario_keepalive.sh - 大量空闲连接 + 突发请求
# 用途: 模拟真实场景中大量长连接中只有少量活跃的模型
# 特点: 高连接数（10000+），低活跃度，最能体现 ET vs LT 差异
# ==============================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./benchmark_common.sh

OPTIMIZATION="${1:-unknown}"

check_deps
check_service
init_metadata "keepalive" "$OPTIMIZATION"

print_scene_header "大量空闲连接测试" "高连接数低活跃度，ET vs LT 差异最明显的场景"

JSON_FILE=$(init_json "keepalive" "高连接数低活跃度，ET vs LT 差异最明显的场景")

BENCHMARKS=(
    "2 5000 20"   # 大量连接，轻度活跃
    "2 10000 20"  # 更多连接，少量活跃
    "2 20000 15"  # 极限空闲连接
)

FIRST=true
for bench in "${BENCHMARKS[@]}"; do
    read -r threads connections duration <<< "$bench"

    JSON_FRAG=$(run_test "$threads" "$connections" "$duration")

    if [ "$FIRST" = true ]; then
        FIRST=false
        echo -n "$JSON_FRAG" >> "$JSON_FILE"
    else
        echo "," >> "$JSON_FILE"
        echo -n "$JSON_FRAG" >> "$JSON_FILE"
    fi

    test_pause

done

finish_json "$JSON_FILE"
echo -e "\n${GREEN}空闲连接压测完成！${NC}"