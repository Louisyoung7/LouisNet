#!/bin/bash
# run_all.sh - 三版本顺序压测，一键无人值守运行
# 前置:  bash benchmark/docker/build_all.sh 已构建好三个镜像
# 用法:  bash benchmark/docker/run_all.sh          # 前台看进度
#        nohup bash benchmark/docker/run_all.sh &  # 后台跑，回来看日志
# 结果:  bench-results/<version>/*.json（仓库内，不随 .worktrees 删除丢失）
# 日志:  bench-results/run_all.log
# ==============================================
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RES="$ROOT/bench-results"
LOG="$RES/run_all.log"
mkdir -p "$RES"

IMAGES=(single mainsub et)
declare -A IMG=(
    [single]=louisnet:bench-single
    [mainsub]=louisnet:bench-mainsub
    [et]=louisnet:bench-et
)
declare -A BIN_DIR=(
    [single]=/src/build/release/benchmark
    [mainsub]=/src/build/Release/benchmark
    [et]=/src/build/Release/benchmark
)
SCENARIOS=(baseline keepalive big_payload pipeline rampup)

log() { echo "[$(date '+%F %T')] $*" | tee -a "$LOG"; }

# 切换电源性能模式（power-profiles-daemon，普通用户免 sudo），结束/中断时恢复
ORIG_PROFILE="$(powerprofilesctl get 2>/dev/null || echo balanced)"
restore_profile() { powerprofilesctl set "$ORIG_PROFILE" 2>/dev/null || true; }
trap restore_profile EXIT

if command -v powerprofilesctl >/dev/null; then
    powerprofilesctl set performance
    log "电源模式: $(powerprofilesctl get)（原为 $ORIG_PROFILE，结束后自动恢复）"
fi

log "===== 三版本压测开始 ====="

for v in "${IMAGES[@]}"; do
    out="$RES/$v"
    mkdir -p "$out"

    if ! docker image inspect "${IMG[$v]}" >/dev/null 2>&1; then
        log "[$v] 镜像 ${IMG[$v]} 不存在，跳过（请先构建）"
        continue
    fi

    log "[$v] 启动服务端（绑定 CPU 4-7）"
    docker rm -f "ln-server-$v" >/dev/null 2>&1
    docker run -d --name "ln-server-$v" --network host \
        --cpuset-cpus=4-7 --memory=8g --ulimit nofile=65535:65535 \
        "${IMG[$v]}" "${BIN_DIR[$v]}/louis_net_benchmark" >>"$LOG" 2>&1
    sleep 3  # 服务端预热

    for s in "${SCENARIOS[@]}"; do
        log "[$v] 场景 $s 运行中..."
        if docker run --rm --network host --cpuset-cpus=8-15 \
            --ulimit nofile=65535:65535 \
            -v "$out:/opt/bench/benchmark_results" \
            "${IMG[$v]}" "/opt/bench/scenario_$s.sh" "$v" >>"$LOG" 2>&1; then
            log "[$v] 场景 $s 完成"
        else
            log "[$v] 场景 $s 失败，继续下一个场景"
        fi
        sleep 2
    done

    docker rm -f "ln-server-$v" >/dev/null 2>&1
    log "[$v] 完成，JSON 在 $out/"
done

log "===== 全部结束，结果在 $RES/<version>/，日志在 $LOG ====="
