#!/usr/bin/env python3
"""
LouisNet 压测结果分析脚本
功能：
  1. 列出所有历史压测结果
  2. 对比两次压测的性能差异
  3. 生成对比报告
"""

import json
import argparse
import os
from pathlib import Path
from datetime import datetime


BENCHMARK_DIR = Path(__file__).parent / "benchmark_results"


def list_results():
    """列出所有压测结果"""
    if not BENCHMARK_DIR.exists():
        print(f"结果目录不存在: {BENCHMARK_DIR}")
        return

    files = sorted(BENCHMARK_DIR.glob("*.json"))
    if not files:
        print("没有找到压测结果文件")
        return

    print(f"\n{'='*70}")
    print(f"压测结果列表 ({BENCHMARK_DIR})")
    print(f"{'='*70}")
    print(f"{'序号':<4} {'时间':<20} {'优化标记':<25} {'RPS范围':<20}")
    print(f"{'-'*70}")

    for i, f in enumerate(files, 1):
        try:
            with open(f) as fp:
                data = json.load(fp)
            timestamp = data.get("timestamp", f.stem)
            opt = data.get("optimization", "unknown")
            rps_values = [t["rps"] for t in data.get("tests", [])]
            rps_range = f"{min(rps_values):.0f} ~ {max(rps_values):.0f}" if rps_values else "N/A"
            print(f"{i:<4} {timestamp:<20} {opt:<25} {rps_range}")
        except Exception as e:
            print(f"{i:<4} {f.name:<20} (解析失败: {e})")

    print(f"{'='*70}\n")
    return files


def load_result(filepath):
    """加载单个压测结果"""
    with open(filepath) as fp:
        return json.load(fp)


def compare(file1_idx, file2_idx):
    """对比两次压测结果"""
    files = sorted(BENCHMARK_DIR.glob("*.json"))

    if not files:
        print("没有找到压测结果文件")
        return

    if file1_idx < 1 or file2_idx < 1 or file1_idx > len(files) or file2_idx > len(files):
        print(f"序号超出范围 (1 ~ {len(files)})")
        return

    data1 = load_result(files[file1_idx - 1])
    data2 = load_result(files[file2_idx - 1])

    print(f"\n{'='*70}")
    print(f"压测结果对比")
    print(f"{'='*70}")
    print(f"  旧: {data1['timestamp']} - {data1['optimization']}")
    print(f"  新: {data2['timestamp']} - {data2['optimization']}")
    print(f"{'='*70}")

    # 按配置匹配对比
    for test1 in data1["tests"]:
        for test2 in data2["tests"]:
            if (test1["threads"] == test2["threads"] and
                test1["connections"] == test2["connections"]):

                threads = test1["threads"]
                conns = test1["connections"]

                # RPS 对比
                rps1, rps2 = test1["rps"], test2["rps"]
                rps_diff = ((rps2 - rps1) / rps1 * 100) if rps1 > 0 else 0
                rps_arrow = "↑" if rps_diff > 0 else "↓"

                # 延迟对比
                lat1, lat2 = test1["avg_latency_us"], test2["avg_latency_us"]
                lat_diff = ((lat2 - lat1) / lat1 * 100) if lat1 > 0 else 0
                lat_arrow = "↓" if lat_diff < 0 else "↑"  # 延迟降低是好事

                print(f"\n[{threads}线程 / {conns}连接]")
                print(f"  RPS:        {rps1:>10,.0f} → {rps2:>10,.0f}  ({rps_arrow}{abs(rps_diff):.1f}%)")
                print(f"  平均延迟:   {lat1:>10,.0f}μs → {lat2:>10,.0f}μs  ({lat_arrow}{abs(lat_diff):.1f}%)")
                print(f"  P99延迟:    {test1['p99_latency_us']:>10,.0f}μs → {test2['p99_latency_us']:>10,.0f}μs")
                print(f"  超时数:     {test1['timeout_count']:>10} → {test2['timeout_count']:>10}")

    print(f"\n{'='*70}\n")


def show_detail(idx):
    """显示单次压测详情"""
    files = sorted(BENCHMARK_DIR.glob("*.json"))

    if not files:
        print("没有找到压测结果文件")
        return

    if idx < 1 or idx > len(files):
        print(f"序号超出范围 (1 ~ {len(files)})")
        return

    data = load_result(files[idx - 1])

    print(f"\n{'='*70}")
    print(f"压测详情 - {data['timestamp']}")
    print(f"{'='*70}")
    print(f"  优化标记: {data['optimization']}")
    print(f"  Git: {data.get('git_commit', 'N/A')[:8]} ({data.get('git_branch', 'N/A')})")
    print(f"\n{'配置':<12} {'RPS':>12} {'平均延迟':>12} {'P99延迟':>12} {'超时':>8}")
    print(f"{'-'*60}")

    for t in data["tests"]:
        print(f"{t['threads']}T/{t['connections']}C"
              f"{t['rps']:>12,.0f}"
              f"{t['avg_latency_us']:>12,.0f}μs"
              f"{t['p99_latency_us']:>12,.0f}μs"
              f"{t['timeout_count']:>8}")

    print(f"{'='*70}\n")


def export_csv():
    """导出所有结果到 CSV"""
    files = sorted(BENCHMARK_DIR.glob("*.json"))
    if not files:
        print("没有找到压测结果文件")
        return

    csv_file = BENCHMARK_DIR / "all_results.csv"
    with open(csv_file, "w") as f:
        f.write("timestamp,optimization,threads,connections,rps,avg_latency_us,p99_latency_us,timeout_count\n")
        for data_file in files:
            data = load_result(data_file)
            for t in data["tests"]:
                f.write(f"{data['timestamp']},{data['optimization']},"
                       f"{t['threads']},{t['connections']},"
                       f"{t['rps']},{t['avg_latency_us']},"
                       f"{t['p99_latency_us']},{t['timeout_count']}\n")

    print(f"已导出 CSV: {csv_file}")


def main():
    parser = argparse.ArgumentParser(description="LouisNet 压测结果分析工具")
    parser.add_argument("-l", "--list", action="store_true", help="列出所有压测结果")
    parser.add_argument("-c", "--compare", nargs=2, type=int, metavar=("ID1", "ID2"),
                       help="对比两次压测结果 (需要指定序号)")
    parser.add_argument("-d", "--detail", type=int, metavar="ID",
                       help="显示单次压测详情 (需要指定序号)")
    parser.add_argument("-e", "--export", action="store_true", help="导出所有结果到 CSV")

    args = parser.parse_args()

    if args.list or not any(vars(args).values()):
        list_results()

    if args.compare:
        compare(args.compare[0], args.compare[1])

    if args.detail:
        show_detail(args.detail)

    if args.export:
        export_csv()


if __name__ == "__main__":
    main()
