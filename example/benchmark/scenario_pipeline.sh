#!/bin/bash
#
# scenario_pipeline.sh - 慢速客户端 / 管道测试
# 用途: 模拟慢速消费者或 HTTP 管道场景
# 特点: 不同 pipeline 深度下观察延迟和吞吐的变化
# ==============================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./benchmark_common.sh

OPTIMIZATION="${1:-unknown}"

check_deps
check_service
init_metadata "pipeline" "$OPTIMIZATION"

print_scene_header "管道/慢速客户端测试" "不同 pipeline 深度下观察延迟和吞吐变化"

JSON_FILE=$(init_json "pipeline" "不同 pipeline 深度下观察延迟和吞吐变化")

# 固定：2 线程，100 连接，持续 15s
# 变量：pipeline 深度 1 / 5 / 10 / 50 / 100
echo -e "  ${YELLOW}说明: pipeline=1 等价于普通 keep-alive 请求${NC}"
echo -e "  ${YELLOW}      pipeline 越深，单次系统调用发送的数据越多${NC}"

PIPELINE_DEPTHS=(1 5 10 20 50 100)

FIRST=true
for depth in "${PIPELINE_DEPTHS[@]}"; do
    JSON_FRAG=$(run_test "2" "100" "15" "--pipeline" "$depth")

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
echo -e "\n${GREEN}管道压测完成！${NC}"