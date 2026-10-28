#!/usr/bin/env python3
"""用 pyecharts 生成可交互的 FPS 折线图 HTML。

CSV 列顺序（无表头时按此顺序读取）：
    instantFps, avgFps, lowFps (1% low), p50, p95, p99

用法:
    python3 scripts/plot_fps_echarts.py test.csv fps.html
    python3 scripts/plot_fps_echarts.py test.csv fps.html --max-points 4000 --smooth 20
"""
import argparse
import csv
import sys

import numpy as np
from pyecharts import options as opts
from pyecharts.charts import Line
from pyecharts.globals import ThemeType

COLUMNS = ["instantFps", "avgFps", "1% low", "p50", "p95", "p99"]
COLORS = ["#f6c445", "#4c9be8", "#e05561", "#59c98b", "#b07de0", "#4ec9c9"]


def load(path):
    with open(path, newline="") as f:
        rows = [r for r in csv.reader(f) if r and len(r) >= 6]
    if not rows:
        sys.exit(f"no data rows in {path}")

    try:
        [float(x) for x in rows[0][:6]]
        start = 0
    except ValueError:
        start = 1

    return np.array([[float(x) for x in r[:6]] for r in rows[start:]])


def rolling_mean(a, window):
    if window <= 1:
        return a
    kernel = np.ones(window) / window
    pad = np.concatenate([np.full(window - 1, a[0]), a])
    return np.convolve(pad, kernel, mode="valid")[: len(a)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input", nargs="?", default="test.csv")
    ap.add_argument("output", nargs="?", default="fps.html")
    ap.add_argument("--max-points", type=int, default=4000,
                    help="最多绘制的点数（0 = 全量）；默认抽稀到 4000 保证交互流畅")
    ap.add_argument("--smooth", type=int, default=1, help="rolling mean window (samples)")
    ap.add_argument("--title", default="OpenMinecraft — FPS / frame time percentiles")
    ap.add_argument("--embed-js", metavar="ECHARTS_JS",
                    help="把 echarts.min.js 内联进输出，生成完全离线可用的单文件 HTML")
    args = ap.parse_args()

    data = load(args.input)
    n = len(data)

    # 抽稀：保留首尾，等步长采样
    if args.max_points and n > args.max_points:
        idx = np.linspace(0, n - 1, args.max_points).astype(int)
        sampled = data[idx]
        x = [int(i) for i in idx]
    else:
        sampled = data
        x = list(range(n))

    line = (
        Line(init_opts=opts.InitOpts(width="100%", height="680px", theme=ThemeType.DARK,
                                     page_title="OpenMinecraft FPS"))
        .add_xaxis(x)
    )

    for i, name in enumerate(COLUMNS):
        y = rolling_mean(sampled[:, i], args.smooth)
        line.add_yaxis(
            series_name=f"{name} (mean {float(np.mean(sampled[:, i])):.0f})",
            y_axis=[round(float(v), 2) for v in y],
            is_smooth=False,
            is_symbol_show=False,
            linestyle_opts=opts.LineStyleOpts(width=2.2 if i < 3 else 1.4, color=COLORS[i]),
            itemstyle_opts=opts.ItemStyleOpts(color=COLORS[i]),
            label_opts=opts.LabelOpts(is_show=False),
        )

    line.set_global_opts(
        title_opts=opts.TitleOpts(title=args.title,
                                  subtitle=f"{n} samples" + (f" · drawn {len(x)}" if len(x) != n else "")
                                           + (f" · smooth {args.smooth}" if args.smooth > 1 else "")),
        legend_opts=opts.LegendOpts(pos_left="center", pos_top="7%", type_="scroll"),
        tooltip_opts=opts.TooltipOpts(trigger="axis", axis_pointer_type="cross"),
        toolbox_opts=opts.ToolboxOpts(is_show=True,
                                      feature={
                                          "dataZoom": {"yAxisIndex": "none"},
                                          "restore": {},
                                          "saveAsImage": {},
                                      }),
        datazoom_opts=[
            opts.DataZoomOpts(type_="inside", xaxis_index=[0], range_start=0, range_end=100),
            opts.DataZoomOpts(type_="slider", xaxis_index=[0], pos_bottom="4%", height=24, range_start=0, range_end=100),
        ],
        xaxis_opts=opts.AxisOpts(name="sample", type_="category", is_scale=True,
                                 name_location="middle", name_gap=32,
                                 axislabel_opts=opts.LabelOpts(rotate=0)),
        yaxis_opts=opts.AxisOpts(name="FPS", is_scale=True, splitline_opts=opts.SplitLineOpts(is_show=True)),
    )

    # 给标题/图例/滑块留出空间，避免与 X 轴刻度重叠（grid 不是 set_global_opts 的参数，
    # 这里直接写入 ECharts 原始配置）
    line.options.update(grid={"top": "20%", "bottom": "20%", "left": "6%", "right": "3%"})

    line.render(args.output)

    if args.embed_js:
        import re
        with open(args.embed_js, encoding="utf-8") as f:
            js = f.read()
        with open(args.output, encoding="utf-8") as f:
            html = f.read()
        html = re.sub(r'<script[^>]*src="[^"]*echarts[^"]*"[^>]*></script>',
                      lambda m: "<script>" + js + "</script>", html, count=1)
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(html)

    print(f"rows={n}  drawn={len(x)}  saved={args.output}")


if __name__ == "__main__":
    main()
