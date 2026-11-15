#!/usr/bin/env python3
"""把 bench-results/<version>/*.json 汇总成 Markdown 对比表。

用法: python3 benchmark/scripts/summarize.py [结果目录，默认 bench-results]
同一版本同一场景有多份结果时，取时间戳最新的一份。
"""

import json
import sys
from pathlib import Path

SCENARIO_ORDER = ["baseline", "keepalive", "big_payload", "pipeline", "rampup"]


def load_latest(root: Path):
    """返回 {(version, scenario): (file, data)}"""
    picked = {}
    for f in sorted(root.glob("*/*.json")):
        try:
            data = json.loads(f.read_text())
        except (json.JSONDecodeError, OSError):
            continue
        key = (f.parent.name, data.get("scenario", "?"))
        # 文件名以时间戳开头，字典序即时间序，后者覆盖前者
        picked[key] = (f, data)
    return picked


def row_label(scenario: str, test: dict, idx: int, dup: bool) -> str:
    conn = test.get("connections", "?")
    depth = test.get("pipeline", 0)
    if scenario == "pipeline" and depth:
        return f"depth={depth}"
    label = f"c{conn}"
    return f"{label}#{idx + 1}" if dup else label


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else "bench-results")
    picked = load_latest(root)
    if not picked:
        print(f"未在 {root}/ 下找到结果 JSON", file=sys.stderr)
        return 1

    versions = sorted({v for v, _ in picked})
    scenarios = [s for s in SCENARIO_ORDER if any(sc == s for _, sc in picked)]
    scenarios += sorted({sc for _, sc in picked} - set(scenarios))

    def cell(version, scenario, idx):
        item = picked.get((version, scenario))
        if not item:
            return None
        tests = item[1]["tests"]
        return tests[idx] if idx < len(tests) else None

    print("# 压测对比汇总\n")
    print(f"版本: {', '.join(versions)}　数据目录: `{root}/`\n")

    for metric, unit, scale in (("rps", "QPS", 1.0), ("timeout_count", "超时", 1.0)):
        print(f"## {unit}\n")
        head = ["场景", "参数"] + versions
        if "single" in versions and "mainsub" in versions:
            head.append("主从/single")
        print("| " + " | ".join(head) + " |")
        print("|" + "|".join(["---"] * len(head)) + "|")

        for scenario in scenarios:
            n = max(
                (len(picked[(v, scenario)][1]["tests"])
                 for v in versions if (v, scenario) in picked),
                default=0,
            )
            conns = [
                cell(v, scenario, i).get("connections")
                for v in versions for i in range(n) if cell(v, scenario, i)
            ]
            # 参数重复时（如 pipeline 旧数据未记录 depth）追加序号区分
            dup = len(conns) != len(set(conns))

            for i in range(n):
                ref = next((cell(v, scenario, i) for v in versions if cell(v, scenario, i)), None)
                if not ref:
                    continue
                label = row_label(scenario, ref, i, dup)
                row = [scenario, label]
                values = {}
                for v in versions:
                    t = cell(v, scenario, i)
                    if t is None:
                        row.append("—")
                        continue
                    if metric == "rps":
                        values[v] = t.get("rps", 0)
                        row.append(f"{values[v] / scale:,.0f}")
                    else:
                        row.append(str(t.get("timeout_count", 0)))
                if "single" in versions and "mainsub" in versions:
                    s, m = values.get("single"), values.get("mainsub")
                    row.append(f"{m / s - 1:+.1%}" if s and m else "—")
                print("| " + " | ".join(row) + " |")
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
