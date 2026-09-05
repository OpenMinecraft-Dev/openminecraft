data = [
1,
0.4153,
0.7326,
6,
2,
0.448,
0.7955,
0.4054,
0.7598,
0.4195,
0.7904,
3,
0.2042,
0.5499,
0.3,
0.3,
0,
2,
2,
0.2668,
0.583,
0.209,
0.5785,
0.2395,
0.5927,
2,
0.3055,
0.5142,
0.2941,
0.5733,
0.3076,
0.5431,
3,
0.4845,
0.6944,
0.195,
0.195,
0,
3,
2,
0.4153,
0.7326,
0.4555,
0.6921,
0.4253,
0.7054
]

import numpy as np
import matplotlib.pyplot as plt
from math import pi, sqrt, atan2, radians

def parse_and_render(data, ax):
    """
    解析扁平浮点数组并绘制。
    数据格式：
      [子路径数量]
      每个子路径：
        起点x, 起点y, 曲线数量
        每条曲线:
          类型: 0直线, 1二次, 2三次, 3圆弧
          对应参数...
    """
    idx = 0
    num_subpaths = int(data[idx]); idx += 1
    
    for _ in range(num_subpaths):
        start_x, start_y = data[idx], data[idx+1]
        curve_count = int(data[idx+2]); idx += 3
        
        current = (start_x, start_y)
        start = (start_x, start_y)
        last_ctrl = start
        
        for _ in range(curve_count):
            ctype = int(data[idx]); idx += 1
            
            if ctype == 0:  # 直线
                ex, ey = data[idx], data[idx+1]; idx += 2
                ax.plot([current[0], ex], [current[1], ey], 'k-', lw=1.5)
                current = (ex, ey)
                last_ctrl = current
                
            elif ctype == 1:  # 二次贝塞尔
                ex, ey = data[idx], data[idx+1]
                cx, cy = data[idx+2], data[idx+3]; idx += 4
                t = np.linspace(0, 1, 50)
                x = (1-t)**2 * current[0] + 2*(1-t)*t * cx + t**2 * ex
                y = (1-t)**2 * current[1] + 2*(1-t)*t * cy + t**2 * ey
                ax.plot(x, y, 'b-', lw=1.5)
                last_ctrl = (cx, cy)
                current = (ex, ey)
                
            elif ctype == 2:  # 三次贝塞尔
                ex, ey = data[idx], data[idx+1]
                c1x, c1y = data[idx+2], data[idx+3]
                c2x, c2y = data[idx+4], data[idx+5]; idx += 6
                t = np.linspace(0, 1, 100)
                x = (1-t)**3 * current[0] + 3*(1-t)**2*t * c1x + 3*(1-t)*t**2 * c2x + t**3 * ex
                y = (1-t)**3 * current[1] + 3*(1-t)**2*t * c1y + 3*(1-t)*t**2 * c2y + t**3 * ey
                ax.plot(x, y, 'g-', lw=1.5)
                last_ctrl = (c2x, c2y)
                current = (ex, ey)
                
            elif ctype == 3:  # 圆弧
                ex, ey = data[idx], data[idx+1]
                rx, ry = data[idx+2], data[idx+3]
                xrot = data[idx+4]
                flags = int(data[idx+5]); idx += 6
                large_arc = (flags >> 1) & 1
                sweep = flags & 1
                
                # 绘制椭圆弧
                if rx == 0 or ry == 0:
                    ax.plot([current[0], ex], [current[1], ey], 'r-', lw=1.5)
                else:
                    phi = radians(xrot)
                    cos_phi, sin_phi = np.cos(phi), np.sin(phi)
                    
                    dx = (current[0] - ex) / 2.0
                    dy = (current[1] - ey) / 2.0
                    x1p = cos_phi * dx + sin_phi * dy
                    y1p = -sin_phi * dx + cos_phi * dy
                    
                    lam = (x1p**2 / rx**2) + (y1p**2 / ry**2)
                    if lam > 1:
                        s = sqrt(lam)
                        rx, ry = s * rx, s * ry
                    
                    num = rx**2 * ry**2 - rx**2 * y1p**2 - ry**2 * x1p**2
                    den = rx**2 * y1p**2 + ry**2 * x1p**2
                    coef = sqrt(max(0, num / den))
                    if large_arc == sweep:
                        coef = -coef
                    
                    cxp = coef * (rx * y1p / ry)
                    cyp = -coef * (ry * x1p / rx)
                    
                    cx = cos_phi * cxp - sin_phi * cyp + (current[0] + ex) / 2.0
                    cy = sin_phi * cxp + cos_phi * cyp + (current[1] + ey) / 2.0
                    
                    theta1 = atan2((y1p - cyp) / ry, (x1p - cxp) / rx)
                    theta2 = atan2((-y1p - cyp) / ry, (-x1p - cxp) / rx)
                    delta_theta = theta2 - theta1
                    
                    if not sweep and delta_theta > 0:
                        delta_theta -= 2 * pi
                    elif sweep and delta_theta < 0:
                        delta_theta += 2 * pi
                    
                    t = np.linspace(0, 1, 200) * delta_theta + theta1
                    x = cx + rx * np.cos(t) * cos_phi - ry * np.sin(t) * sin_phi
                    y = cy + rx * np.cos(t) * sin_phi + ry * np.sin(t) * cos_phi
                    ax.plot(x, y, 'r-', lw=1.5)
                
                last_ctrl = current  # 圆弧后，控制点重置为当前点
                current = (ex, ey)
                
            else:
                raise ValueError(f"未知曲线类型 {ctype}")
        
        # 可选：闭合子路径（如果需要可以绘制起点连线，这里不画）


# 绘图
fig, ax = plt.subplots(figsize=(10, 10))
parse_and_render(data, ax)
ax.set_aspect('equal')
ax.invert_yaxis()  # 匹配 SVG 坐标系
ax.grid(True, linestyle='--', alpha=0.3)
plt.show()