#!/bin/bash
# build_all.sh - 按分支名按需创建 worktree 并构建三个压测镜像
# 用法:  bash docker/build_all.sh
# 依赖:  本地存在 http / bench/single-reactor / bench/main-sub-reactor 三个分支
#        克隆仓库后通常只有 main，需先:
#          git fetch origin
#          git checkout -b http origin/http
#          git checkout -b bench/single-reactor origin/bench/single-reactor
#          git checkout -b bench/main-sub-reactor origin/bench/main-sub-reactor
# ==============================================
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
WT="${BENCH_WORKTREE_DIR:-.worktrees}"

ensure_worktree() {  # $1=子目录名 $2=分支名
    local dir="$WT/$1" branch="$2"
    if [ -e "$dir/.git" ]; then
        echo "[$1] worktree 已存在: $dir"
        return 0
    fi
    if ! git show-ref --verify --quiet "refs/heads/$branch"; then
        echo "[$1] 错误: 本地缺少分支 $branch" >&2
        return 1
    fi
    mkdir -p "$WT"
    echo "[$1] 从分支 $branch 创建 worktree: $dir"
    git worktree add "$dir" "$branch"
}

build() {  # $1=版本名 $2=构建上下文目录
    local name="$1" ctx="$2"
    local sha
    sha="$(git -C "$ctx" rev-parse --short HEAD)"
    echo "=== 构建 louisnet:bench-$name (commit $sha) ==="
    docker build -f "docker/Dockerfile.$name" --build-arg GIT_SHA="$sha" \
        -t "louisnet:bench-$name" "$ctx"
}

# 单 Reactor、主从 Reactor 使用独立 worktree
ensure_worktree single-reactor   bench/single-reactor   || exit 1
ensure_worktree main-sub-reactor bench/main-sub-reactor || exit 1

# HEAD（http 分支）：若当前就在 http 上则直接用仓库根，否则单独开 worktree
if [ "$(git rev-parse --abbrev-ref HEAD)" = "http" ]; then
    HEAD_CTX="$ROOT"
else
    ensure_worktree head-http http || exit 1
    HEAD_CTX="$WT/head-http"
fi

build single   "$WT/single-reactor"   || exit 1
build mainsub  "$WT/main-sub-reactor" || exit 1
build head     "$HEAD_CTX"            || exit 1

echo "=== 三个镜像构建完成 ==="
