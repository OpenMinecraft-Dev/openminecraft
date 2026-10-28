import re, sys, json, hashlib

src = sys.argv[1] if len(sys.argv) > 1 else "/tmp/om_sample.txt"
out = sys.argv[2] if len(sys.argv) > 2 else "/tmp/flamegraph.html"
lines = open(src, encoding="utf-8", errors="ignore").read().splitlines()
start = next(i for i, l in enumerate(lines) if l.strip().startswith("Call graph:")) + 1

pat = re.compile(r'^([ +!:]*)(\d+)\s+(.*)$')
root = {"name": "all", "count": 0, "children": []}
stack = []
for l in lines[start:]:
    if not l.strip() or l.startswith("Total number") or l.startswith("Binary Images"):
        continue
    m = pat.match(l)
    if not m:
        continue
    prefix, cnt, rest = m.group(1), int(m.group(2)), m.group(3)
    depth = max(0, (len(prefix) - 4) // 2)
    name = rest.split("  (in ")[0].strip()
    m2 = re.search(r'([\w./+-]+\.(?:cpp|hpp|h|c|cc|mm|m|s|metal|glsl)):(\d+)', rest)
    if m2:
        name += "  [" + m2.group(1).split('/')[-1] + ":" + m2.group(2) + "]"
    while len(stack) > depth:
        stack.pop()
    parent = stack[-1] if stack else root
    node = {"name": name, "count": cnt, "children": [], "self": 0}
    parent["children"].append(node)
    stack.append(node)

def compute(n):
    kids = sum(c["count"] for c in n["children"])
    n["self"] = max(0, n["count"] - kids)
    for c in n["children"]:
        compute(c)
compute(root)

def build(node, depth, x0, total, width):
    """返回 [{name,x,w,d,self}]（不展开 self）以及递归子节点"""
    rects = []
    cursor = x0
    for c in sorted(node["children"], key=lambda z: -z["count"]):
        w = c["count"] / total * width
        if w < 0.15:
            cursor += w
            continue
        rects.append({"n": c["name"], "x": round(cursor, 2), "w": round(w, 2), "d": depth, "v": c["count"]})
        rects += build(c, depth + 1, cursor, total, width)
        cursor += w
    return rects

def dataset(thread_name):
    base = root
    for c in root["children"]:
        if thread_name and thread_name in c["name"]:
            base = c
            break
    total = base["count"] or 1
    rects = build(base, 0, 0, total, 1000.0)
    return {"name": thread_name or "all threads", "total": total, "rects": rects}

data = {"main": dataset("com.apple.main-thread"), "all": dataset(None)}

html = """<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="utf-8">
<title>OpenMinecraft — Interactive Flame Graph</title>
<style>
  :root { color-scheme: dark; }
  body { margin: 0; background: #1c1e24; color: #e6e6e6; font: 13px -apple-system, "Helvetica Neue", Arial, sans-serif; }
  header { display: flex; gap: 12px; align-items: center; padding: 10px 14px; background: #23252c; border-bottom: 1px solid #34363f; position: sticky; top: 0; z-index: 10; }
  header b { font-weight: 600; }
  input[type=text] { background: #2c2f37; border: 1px solid #454854; color: #e6e6e6; border-radius: 4px; padding: 4px 8px; width: 240px; }
  button, select { background: #30333c; border: 1px solid #454854; color: #e6e6e6; border-radius: 4px; padding: 4px 10px; cursor: pointer; }
  button:hover { background: #3a3e49; }
  .hint { color: #9aa0ad; font-size: 12px; }
  #wrap { position: relative; }
  #chart { display: block; width: 100%; height: calc(100vh - 58px); }
  rect.frame { cursor: pointer; stroke: #10121a; stroke-width: 0.4; }
  rect.frame:hover { stroke: #fff; stroke-width: 1.2; }
  rect.hit { fill: #ffd166 !important; }
  rect.dim { opacity: 0.25; }
  text.label { fill: #10121a; font-size: 11px; pointer-events: none; dominant-baseline: middle; }
  #tip { position: fixed; pointer-events: none; background: #0e1014ee; border: 1px solid #4a4d59; border-radius: 4px; padding: 6px 8px; font-size: 12px; max-width: 640px; display: none; z-index: 20; }
  #status { color: #9aa0ad; font-size: 12px; }
</style>
</head>
<body>
<header>
  <b>Flame Graph</b>
  <select id="ds">
    <option value="main">main thread</option>
    <option value="all">all threads</option>
  </select>
  <input id="q" type="text" placeholder="搜索函数名 / 文件名…">
  <button id="reset">重置缩放</button>
  <span id="status"></span>
  <span class="hint">点击帧缩放 · 悬停查看耗时 · 双击/重置返回</span>
</header>
<div id="wrap"><svg id="chart"></svg></div>
<div id="tip"></div>
<script>
const DATA = __DATA__;
const SVG = document.getElementById('chart');
let cur = 'main';
let zoom = null;          // {x, w, d} 当前缩放窗口
let rects = [];           // 当前布局结果
let totalCount = 0;
const colorCache = new Map();

function hash(s){let h=0;for(let i=0;i<s.length;i++){h=(h*31+s.charCodeAt(i))|0;}return Math.abs(h);}
function color(name){
  if(colorCache.has(name)) return colorCache.get(name);
  const h = hash(name) % 360;
  const c = `hsl(${h} 55% 72%)`;
  colorCache.set(name, c);
  return c;
}
function fmt(n){ return n.toLocaleString(); }

function layout(){
  const ds = DATA[cur];
  totalCount = ds.total;
  const W = 1000;                       // 逻辑宽度
  const H = 18;                         // 每层高度
  rects = [];
  const win = zoom;                     // {x0, x1, d0}
  for(const r of ds.rects){
    if(win){ if(r.d <= win.d0) continue; if(r.x + r.w <= win.x0 || r.x >= win.x1) continue; }
    let x = r.x, w = r.w, d = r.d;
    if(win){ x = (r.x - win.x0) / (win.x1 - win.x0) * W; w = r.w / (win.x1 - win.x0) * W; d = r.d - win.d0 - 1; }
    rects.push({n:r.n, x, w, d, v:r.v, ox:r.x, ow:r.w, od:r.d});
  }
  const depth = rects.reduce((m,r)=>Math.max(m, r.d+1), 1);
  SVG.setAttribute('viewBox', `0 0 ${W} ${depth*H + 4}`);
  SVG.setAttribute('height', Math.max(300, depth*H + 60));
  SVG.innerHTML = '';

  const q = document.getElementById('q').value.trim().toLowerCase();
  for(const r of rects){
    const g = document.createElementNS('http://www.w3.org/2000/svg','g');
    const rect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    rect.setAttribute('x', r.x); rect.setAttribute('y', r.d*H + 2);
    rect.setAttribute('width', Math.max(r.w - 0.3, 0.3)); rect.setAttribute('height', H - 1.5);
    rect.setAttribute('rx', 1.5);
    rect.setAttribute('class', 'frame' + (q && r.n.toLowerCase().includes(q) ? ' hit' : (q ? ' dim' : '')));
    rect.setAttribute('fill', color(r.n));
    const title = document.createElementNS('http://www.w3.org/2000/svg','title');
    const pct = (r.v / totalCount * 100).toFixed(2);
    title.textContent = `${r.n}\n${fmt(r.v)} samples (${pct}%)`;
    rect.appendChild(title);
    rect.addEventListener('click', (ev)=>{ ev.stopPropagation(); zoomTo(r); });
    g.appendChild(rect);
    if(r.w > 34){
      const t = document.createElementNS('http://www.w3.org/2000/svg','text');
      t.setAttribute('x', r.x + 3); t.setAttribute('y', r.d*H + 2 + (H-1.5)/2);
      t.setAttribute('class','label');
      const maxChars = Math.max(3, Math.floor(r.w / 5.2));
      t.textContent = r.n.length > maxChars ? r.n.slice(0, maxChars-1) + '…' : r.n;
      g.appendChild(t);
    }
    SVG.appendChild(g);
  }
  document.getElementById('status').textContent =
    `${ds.name} · ${fmt(totalCount)} samples` + (zoom ? ' · zoomed' : '');
}

function zoomTo(r){
  // 直接用布局时保存的原始坐标，避免坐标二次换算
  zoom = { x0: r.ox, x1: r.ox + r.ow, d0: r.od };
  layout();
}

document.getElementById('reset').addEventListener('click', ()=>{ zoom = null; layout(); });
document.getElementById('ds').addEventListener('change', (e)=>{ cur = e.target.value; zoom = null; layout(); });
document.getElementById('q').addEventListener('input', ()=> layout());
SVG.addEventListener('dblclick', ()=>{ zoom = null; layout(); });
document.addEventListener('keydown', (e)=>{ if(e.key === 'Escape'){ zoom = null; layout(); } });
layout();
</script>
</body>
</html>
"""
html = html.replace("__DATA__", json.dumps(data, ensure_ascii=False))
open(out, "w", encoding="utf-8").write(html)
print("saved", out, len(html), "bytes; main rects:", len(data["main"]["rects"]), "all rects:", len(data["all"]["rects"]))
