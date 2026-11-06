#!/usr/bin/env python3
"""用 pyecharts 生成可交互的 FPS 折线图 HTML。

CSV 列顺序（无表头时按此顺序读取）：
    instantFps, avgFps, lowFps (1% low), p50, p95, p99

横坐标为**时间（秒）**：由每帧 instantFps 换算帧时长后累加得到（起点 0）。

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


def build_time_axis(data):
    """用 instantFps 还原每帧耗时并累加，得到秒级时间轴（相对起点 0）。"""
    fps = np.maximum(data[:, 0], 1.0)
    frame_s = 1.0 / fps
    return np.cumsum(frame_s) - frame_s[0]


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

    # 全量时间轴（秒），抽稀后再取值，保证时间间隔正确
    time_s = build_time_axis(data)

    if args.max_points and n > args.max_points:
        idx = np.linspace(0, n - 1, args.max_points).astype(int)
        sampled = data[idx]
        x = time_s[idx]
    else:
        sampled = data
        x = time_s

    line = (
        Line(init_opts=opts.InitOpts(width="100%", height="680px", theme=ThemeType.DARK,
                                     page_title="OpenMinecraft FPS"))
        # 先给一个非空的 x 轴，避免 pyecharts 丢弃 series；实际坐标用 [x, y] 数据对
        .add_xaxis([f"{v:.3f}" for v in x])
    )

    all_points = []

    for i, name in enumerate(COLUMNS):
        y = rolling_mean(sampled[:, i], args.smooth)
        all_points.append([[round(float(xi), 4), round(float(yi), 2)] for xi, yi in zip(x, y)])
        line.add_yaxis(
            series_name=f"{name} (mean {float(np.mean(sampled[:, i])):.0f})",
            y_axis=[],
            is_smooth=False,
            is_symbol_show=False,
            linestyle_opts=opts.LineStyleOpts(width=2.2 if i < 3 else 1.4, color=COLORS[i]),
            itemstyle_opts=opts.ItemStyleOpts(color=COLORS[i]),
            label_opts=opts.LabelOpts(is_show=False),
        )

    # 直接写入数值对，规避 pyecharts 对 x 轴数据的自动打包
    for si, pts in enumerate(all_points):
        line.options["series"][si]["data"] = pts

    line.set_global_opts(
        title_opts=opts.TitleOpts(title=args.title,
                                  subtitle=f"{n} samples · span {float(x[-1]):.1f}s"
                                           + (f" · drawn {len(x)}" if len(x) != n else "")
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
        xaxis_opts=opts.AxisOpts(name="time (s)", type_="value", is_scale=True,
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
