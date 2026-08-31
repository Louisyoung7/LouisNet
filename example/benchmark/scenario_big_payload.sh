#!/bin/bash
#
# scenario_big_payload.sh - 大包传输测试
# 用途: 测试网络库对大消息体的处理能力
# 特点: 使用 wrk 的 pipeline 模拟大请求场景，或自定义 lua 脚本发送大 payload
#       建议同时配合服务端 /big 端点（返回大响应体）使用
# ==============================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./benchmark_common.sh

OPTIMIZATION="${1:-unknown}"

check_deps
check_service
init_metadata "big_payload" "$OPTIMIZATION"

print_scene_header "大包传输测试" "测试大消息体下的吞吐能力，readv 和 ET 在此场景下收益更明显"

JSON_FILE=$(init_json "big_payload" "测试大消息体下的吞吐能力")

# ---- 场景 A: 普通请求，大响应体（需服务端支持 /big 返回大包） ----
echo -e "\n${YELLOW}[场景A] 大响应体测试（需服务端支持 /big 端点）${NC}"

# 尝试测试 /big 端点，如果服务端不支持则跳过
if curl -sf --connect-timeout 2 -o /dev/null "${HOST}:${PORT}/big" 2>/dev/null; then
    BIG_URL="http://${HOST}:${PORT}/big"
    BENCHMARKS_BIG=(
        "2 100 15"
        "4 500 15"
    )

    for bench in "${BENCHMARKS_BIG[@]}"; do
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
else
    echo -e "  ${YELLOW}跳过: /big 端点不可用${NC}"
fi

# ---- 场景 B: 高管道深度 = 大包效果 ----
echo -e "\n${YELLOW}[场景B] HTTP Pipeline 深度测试${NC}"
echo -e "  ${YELLOW}说明: wrk --pipeline 参数模拟一次性发送多个请求，等效于大包传输${NC}"

PIPELINE_DEPTHS=(1 10 50 100)
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
echo -e "\n${GREEN}大包传输压测完成！${NC}"