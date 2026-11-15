#!/usr/bin/env python3
"""把 bench-results/<version>/*.json 生成自包含的可视化报告 report.html。

用法: python3 benchmark/scripts/report.py [结果目录，默认 bench-results]
输出: <结果目录>/report.html（数据内嵌，离线可看，无需起服务）
同一版本同一场景有多份结果时，取时间戳最新的一份。
"""

import json
import sys
from datetime import datetime
from pathlib import Path

SCENARIO_ORDER = ["baseline", "keepalive", "big_payload", "pipeline", "rampup"]
SCENARIO_NAMES = {
    "baseline": "标准并发",
    "keepalive": "长连接",
    "big_payload": "大响应体",
    "pipeline": "管道",
    "rampup": "阶梯加压",
}


def load_latest(root: Path):
    picked = {}
    for f in sorted(root.glob("*/*.json")):
        try:
            data = json.loads(f.read_text())
        except (json.JSONDecodeError, OSError):
            continue
        key = (f.parent.name, data.get("scenario", "?"))
        picked[key] = (f, data)
    return picked


def build_payload(root: Path, picked):
    versions = sorted({v for v, _ in picked})
    scenarios = [s for s in SCENARIO_ORDER if any(sc == s for _, sc in picked)]
    scenarios += sorted({sc for _, sc in picked} - set(scenarios))

    series = []
    for scenario in scenarios:
        n = max(
            (len(picked[(v, scenario)][1]["tests"])
             for v in versions if (v, scenario) in picked),
            default=0,
        )
        rows = []
        labels = []
        for i in range(n):
            ref = next(
                (picked[(v, scenario)][1]["tests"][i]
                 for v in versions
                 if (v, scenario) in picked and i < len(picked[(v, scenario)][1]["tests"])),
                None,
            )
            if not ref:
                continue
            depth = ref.get("pipeline", 0)
            base = f"depth={depth}" if scenario == "pipeline" and depth else f"c{ref.get('connections', '?')}"
            labels.append(base)
            row = {"label": base, "values": {}}
            for v in versions:
                item = picked.get((v, scenario))
                if not item or i >= len(item[1]["tests"]):
                    continue
                t = item[1]["tests"][i]
                row["values"][v] = {
                    "rps": t.get("rps", 0),
                    "avg": t.get("avg_latency_us", 0),
                    "p99": t.get("p99_latency_us", 0),
                    "timeouts": t.get("timeout_count", 0),
                }
            rows.append(row)

        # 标签重复时追加序号（旧数据未记录 pipeline 深度的情况）
        if len(labels) != len(set(labels)):
            seen = {}
            for row in rows:
                seen[row["label"]] = seen.get(row["label"], 0) + 1
                row["label"] = f"{row['label']}#{seen[row['label']]}"

        series.append({"scenario": scenario, "name": SCENARIO_NAMES.get(scenario, scenario), "rows": rows})

    return {"versions": versions, "series": series, "generated": datetime.now().strftime("%Y-%m-%d %H:%M"), "source": str(root)}


HTML = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LouisNet 压测对比</title>
<style>
:root {
  --bg: #FAFAFA;
  --surface: #FFFFFF;
  --surface-muted: #F4F4F5;
  --border: #E4E4E7;
  --text: #18181B;
  --text-muted: #71717A;
  --s1: #3B82F6;
  --s2: #10B981;
  --s3: #8B5CF6;
  --warn: #DC2626;
  --radius: 8px;
  --radius-card: 12px;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #111113;
    --surface: #18181B;
    --surface-muted: #232327;
    --border: #2E2E33;
    --text: #E4E4E7;
    --text-muted: #A1A1AA;
    --s1: #60A5FA;
    --s2: #34D399;
    --s3: #A78BFA;
    --warn: #F87171;
  }
}
* { box-sizing: border-box; }
body {
  margin: 0; padding: 24px;
  background: var(--bg); color: var(--text);
  font: 14px/1.6 -apple-system, "PingFang SC", "Segoe UI", sans-serif;
}
.wrap { max-width: 1120px; margin: 0 auto; }
h1 { font-size: 20px; margin: 0 0 4px; font-weight: 600; }
.meta { color: var(--text-muted); font-size: 12px; margin-bottom: 20px; }
.card {
  background: var(--surface); border: 1px solid var(--border);
  border-radius: var(--radius-card); padding: 16px 20px; margin-bottom: 16px;
}
h2 { font-size: 15px; font-weight: 600; margin: 0 0 12px; }
.kpis { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; margin-bottom: 16px; }
.kpi { background: var(--surface); border: 1px solid var(--border); border-radius: var(--radius-card); padding: 14px 16px; }
.kpi .name { font-size: 12px; color: var(--text-muted); display: flex; align-items: center; gap: 6px; }
.kpi .dot { width: 8px; height: 8px; border-radius: 999px; }
.kpi .val { font-size: 20px; font-weight: 600; margin-top: 4px; font-variant-numeric: tabular-nums; }
.kpi .sub { font-size: 12px; color: var(--text-muted); }
.tabs { display: flex; gap: 6px; flex-wrap: wrap; margin-bottom: 14px; }
.tab {
  border: 1px solid var(--border); background: var(--surface-muted); color: var(--text);
  border-radius: 999px; padding: 5px 14px; font-size: 13px; cursor: pointer;
}
.tab.active { background: var(--text); color: var(--bg); border-color: var(--text); }
.chart { width: 100%; height: auto; display: block; }
.legend { display: flex; gap: 16px; flex-wrap: wrap; margin-top: 10px; font-size: 12px; color: var(--text-muted); }
.legend span { display: inline-flex; align-items: center; gap: 6px; }
table { width: 100%; border-collapse: collapse; font-variant-numeric: tabular-nums; }
th, td { padding: 7px 10px; text-align: right; border-bottom: 1px solid var(--border); font-size: 13px; }
th:first-child, td:first-child { text-align: left; }
th { color: var(--text-muted); font-weight: 500; font-size: 12px; white-space: nowrap; }
td.timeout { color: var(--warn); font-weight: 600; }
td.delta { color: var(--text-muted); }
.note { font-size: 12px; color: var(--text-muted); margin-top: 10px; }
.empty { color: var(--text-muted); padding: 20px 0; }
</style>
</head>
<body>
<div class="wrap">
  <h1>LouisNet 压测对比</h1>
  <div class="meta" id="meta"></div>
  <div class="kpis" id="kpis"></div>
  <div class="card">
    <h2>吞吐对比（QPS）</h2>
    <div class="tabs" id="tabs"></div>
    <svg class="chart" id="chart" viewBox="0 0 960 400" preserveAspectRatio="xMidYMid meet"></svg>
    <div class="legend" id="legend"></div>
    <div class="note">纵轴为每秒请求数；悬停柱体查看平均/p99 延迟与超时数。</div>
  </div>
  <div class="card">
    <h2>明细</h2>
    <div id="table"></div>
  </div>
</div>
<script>
const DATA = __DATA__;
const COLORS = ["var(--s1)", "var(--s2)", "var(--s3)"];
const colorOf = (i) => COLORS[i % COLORS.length];
let active = DATA.series.length ? DATA.series[0].scenario : null;

function fmt(n) { return n.toLocaleString("en-US", { maximumFractionDigits: 0 }); }
function fmtUs(us) { return us >= 1000 ? (us / 1000).toFixed(2) + " ms" : us.toFixed(1) + " µs"; }

document.getElementById("meta").textContent =
  "版本: " + DATA.versions.join(" / ") + "　·　数据目录: " + DATA.source + "　·　生成时间: " + DATA.generated;

function renderKpis() {
  const box = document.getElementById("kpis");
  box.innerHTML = "";
  DATA.versions.forEach((v, vi) => {
    let peak = 0, peakLabel = "—", timeouts = 0;
    DATA.series.forEach(s => s.rows.forEach(r => {
      const d = r.values[v];
      if (!d) return;
      if (d.rps > peak) { peak = d.rps; peakLabel = s.name + " " + r.label; }
      timeouts += d.timeouts;
    }));
    const el = document.createElement("div");
    el.className = "kpi";
    el.innerHTML = '<div class="name"><span class="dot" style="background:' + colorOf(vi) + '"></span>' + v + '</div>'
      + '<div class="val">' + fmt(peak) + '</div>'
      + '<div class="sub">峰值 QPS · ' + peakLabel + '　超时合计 ' + fmt(timeouts) + '</div>';
    box.appendChild(el);
  });
}

function renderTabs() {
  const box = document.getElementById("tabs");
  box.innerHTML = "";
  DATA.series.forEach(s => {
    const b = document.createElement("button");
    b.className = "tab" + (s.scenario === active ? " active" : "");
    b.textContent = s.name;
    b.onclick = () => { active = s.scenario; renderTabs(); renderChart(); };
    box.appendChild(b);
  });
}

function renderChart() {
  const s = DATA.series.find(x => x.scenario === active);
  const svg = document.getElementById("chart");
  svg.innerHTML = "";
  const NS = "http://www.w3.org/2000/svg";
  const W = 960, H = 400, L = 62, R = 16, T = 14, B = 52;
  const plotW = W - L - R, plotH = H - T - B;
  document.getElementById("legend").innerHTML = DATA.versions.map((v, i) =>
    '<span><span class="dot" style="width:8px;height:8px;border-radius:999px;display:inline-block;background:' + colorOf(i) + '"></span>' + v + '</span>'
  ).join("");

  if (!s || !s.rows.length) {
    const t = document.createElementNS(NS, "text");
    t.setAttribute("x", W / 2); t.setAttribute("y", H / 2);
    t.setAttribute("text-anchor", "middle"); t.setAttribute("fill", "var(--text-muted)");
    t.textContent = "该场景无数据";
    svg.appendChild(t);
    return;
  }

  const max = Math.max(...s.rows.flatMap(r => DATA.versions.map(v => (r.values[v] || {}).rps || 0))) * 1.08 || 1;

  // 网格与刻度
  for (let g = 0; g <= 4; g++) {
    const y = T + plotH - (plotH * g / 4);
    const line = document.createElementNS(NS, "line");
    line.setAttribute("x1", L); line.setAttribute("x2", W - R);
    line.setAttribute("y1", y); line.setAttribute("y2", y);
    line.setAttribute("stroke", "var(--border)");
    svg.appendChild(line);
    const label = document.createElementNS(NS, "text");
    label.setAttribute("x", L - 8); label.setAttribute("y", y + 4);
    label.setAttribute("text-anchor", "end"); label.setAttribute("fill", "var(--text-muted)");
    label.setAttribute("font-size", "11");
    label.textContent = fmt(max * g / 4);
    svg.appendChild(label);
  }

  const groupW = plotW / s.rows.length;
  const barW = Math.min(28, (groupW - 18) / DATA.versions.length);
  s.rows.forEach((row, ri) => {
    const gx = L + groupW * ri + groupW / 2;
    const totalW = barW * DATA.versions.length + 4 * (DATA.versions.length - 1);
    DATA.versions.forEach((v, vi) => {
      const d = row.values[v];
      const x = gx - totalW / 2 + vi * (barW + 4);
      const h = d ? Math.max(0, plotH * d.rps / max) : 0;
      if (d) {
        const rect = document.createElementNS(NS, "rect");
        rect.setAttribute("x", x); rect.setAttribute("y", T + plotH - h);
        rect.setAttribute("width", barW); rect.setAttribute("height", h);
        rect.setAttribute("rx", 3); rect.setAttribute("fill", colorOf(vi));
        const tip = document.createElementNS(NS, "title");
        tip.textContent = row.label + " · " + v + "\\nQPS " + fmt(d.rps)
          + "\\n平均 " + fmtUs(d.avg) + " · p99 " + fmtUs(d.p99)
          + "\\n超时 " + d.timeouts;
        rect.appendChild(tip);
        svg.appendChild(rect);
      }
      if (vi === 0) {
        const lb = document.createElementNS(NS, "text");
        lb.setAttribute("x", gx); lb.setAttribute("y", H - B + 20);
        lb.setAttribute("text-anchor", "middle"); lb.setAttribute("fill", "var(--text)");
        lb.setAttribute("font-size", "12");
        lb.textContent = row.label;
        svg.appendChild(lb);
      }
    });
  });
}

function renderTable() {
  const s = DATA.series.find(x => x.scenario === active);
  const box = document.getElementById("table");
  if (!s || !s.rows.length) { box.innerHTML = '<div class="empty">无数据</div>'; return; }
  const vs = DATA.versions;
  let html = "<table><thead><tr><th>参数</th>";
  vs.forEach(v => { html += "<th>" + v + " QPS</th>"; });
  if (vs.includes("single") && vs.includes("mainsub")) html += "<th>主从/single</th>";
  vs.forEach(v => { html += "<th>" + v + " 超时</th>"; });
  html += "</tr></thead><tbody>";
  s.rows.forEach(row => {
    html += "<tr><td>" + row.label + "</td>";
    vs.forEach(v => {
      const d = row.values[v];
      html += "<td>" + (d ? fmt(d.rps) : "—") + "</td>";
    });
    if (vs.includes("single") && vs.includes("mainsub")) {
      const a = row.values["single"], b = row.values["mainsub"];
      html += '<td class="delta">' + (a && b && a.rps ? (b.rps / a.rps - 1 >= 0 ? "+" : "") + ((b.rps / a.rps - 1) * 100).toFixed(1) + "%" : "—") + "</td>";
    }
    vs.forEach(v => {
      const d = row.values[v];
      html += d && d.timeouts > 0 ? '<td class="timeout">' + fmt(d.timeouts) + "</td>" : "<td>0</td>";
    });
    html += "</tr>";
  });
  html += "</tbody></table>";
  box.innerHTML = html;
}

renderKpis();
renderTabs();
renderChart();
renderTable();
</script>
</body>
</html>
"""


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else "bench-results")
    picked = load_latest(root)
    if not picked:
        print(f"未在 {root}/ 下找到结果 JSON", file=sys.stderr)
        return 1
    payload = build_payload(root, picked)
    out = root / "report.html"
    out.write_text(HTML.replace("__DATA__", json.dumps(payload, ensure_ascii=False)), encoding="utf-8")
    print(f"已生成 {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
