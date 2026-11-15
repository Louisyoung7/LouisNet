#!/bin/bash
#
# scenario_baseline.sh - 基线性能测试
# 用途: 对比不同优化/不同 epoll 模式下的基准性能
# 特点: 标准 wrk 压测，覆盖低/中/高并发
# ==============================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

source ./benchmark_common.sh

OPTIMIZATION="${1:-unknown}"

# 检查

check_deps
check_service
init_metadata "baseline" "$OPTIMIZATION"

# 打印标题
print_scene_header "基线性能测试" "标准 wrk 压测，覆盖低/中/高并发场景"

# 初始化 JSON
JSON_FILE=$(init_json "baseline" "标准 wrk 压测，覆盖低/中/高并发场景")

# 测试用例：线程 连接 持续时间
BENCHMARKS=(
    "1 100 10"    # 基线：低并发
    "2 500 30"    # 稳定性：中等并发 + 长时间
    "2 1000 10"   # 中等并发
    "4 2000 15"   # 高并发
    "8 4000 15"   # 极限压力
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
echo -e "\n${GREEN}基线压测完成！${NC}"