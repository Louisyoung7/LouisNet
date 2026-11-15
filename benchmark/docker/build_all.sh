#!/bin/bash
# build_all.sh - 按分支名按需创建 worktree 并构建三个压测镜像
# 用法:  bash benchmark/docker/build_all.sh
# 依赖: 三个版本分支 http / bench/single-reactor / bench/main-sub-reactor
#        本地缺失时会自动从 origin 建立跟踪分支（克隆仓库后可直接运行）
# ==============================================
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
WT="${BENCH_WORKTREE_DIR:-.worktrees}"

ensure_branch() {  # $1=分支名：本地没有则从 origin 建立跟踪分支
    local branch="$1"
    git show-ref --verify --quiet "refs/heads/$branch" && return 0
    if git show-ref --verify --quiet "refs/remotes/origin/$branch"; then
        echo "[$branch] 本地缺失，从 origin/$branch 建立跟踪分支"
        git branch --track "$branch" "origin/$branch"
    else
        echo "错误: 本地和远端都没有分支 $branch" >&2
        return 1
    fi
}

ensure_worktree() {  # $1=子目录名 $2=分支名
    local dir="$WT/$1" branch="$2"
    if [ -e "$dir/.git" ]; then
        echo "[$1] worktree 已存在: $dir"
        return 0
    fi
    ensure_branch "$branch" || return 1
    mkdir -p "$WT"
    echo "[$1] 从分支 $branch 创建 worktree: $dir"
    git worktree add "$dir" "$branch"
}

build() {  # $1=版本名 $2=构建上下文目录
    local name="$1" ctx="$2"
    local sha
    sha="$(git -C "$ctx" rev-parse --short HEAD)"
    echo "=== 构建 louisnet:bench-$name (commit $sha) ==="
    docker build -f "benchmark/docker/Dockerfile.$name" --build-arg GIT_SHA="$sha" \
        -t "louisnet:bench-$name" "$ctx"
}

# 单 Reactor、主从 Reactor 使用独立 worktree
ensure_worktree single-reactor   bench/single-reactor   || exit 1
ensure_worktree main-sub-reactor bench/main-sub-reactor || exit 1

build single   "$WT/single-reactor"   || exit 1
build mainsub  "$WT/main-sub-reactor" || exit 1
# 第三个版本（ET）：直接构建当前分支的工作区
build et       "$ROOT"                || exit 1

echo "=== 三个镜像构建完成 ==="
