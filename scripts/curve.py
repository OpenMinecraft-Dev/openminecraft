data = [
4,
3.25,4.8,14,
2,4.205,4.929,3.765,4.8,4.023,4.8,
1,4.371,5.095,4.302,4.998,
2,4.5,6.05,4.501,5.277,4.5,5.535,
0,4.5,6.75,
2,4.371,7.705,4.5,7.265,4.5,7.523,
3,4.205,7.871,0.7,0.7,0,1,
2,3.25,8,4.023,8.001,3.765,8,
2,2.295,7.871,2.735,8,2.477,8,
3,2.129,7.705,0.7,0.7,0,1,
2,2,6.75,1.999,7.523,2,7.265,
0,2,6.05,
2,2.129,5.095,2,5.535,2,5.277,
3,2.295,4.929,0.7,0.7,0,1,
2,3.25,4.799,2.477,4.799,2.735,4.799,
7.1,6.2,14,
2,7.794,6.296,7.475,6.2,7.663,6.2,
3,7.904,6.406,0.5,0.5,0,1,
2,8,7.1,8,6.538,8,6.725,
2,7.904,7.794,8,7.475,8,7.663,
3,7.794,7.904,0.5,0.5,0,1,
2,7.1,8,7.663,8,7.474,8,
0,6.3,8,
2,5.606,7.904,5.925,8,5.738,8,
3,5.496,7.794,0.5,0.5,0,1,
2,5.4,7.1,5.4,7.663,5.4,7.474,
2,5.495,6.406,5.4,6.725,5.4,6.538,
3,5.606,6.296,0.5,0.5,0,1,
2,6.3,6.2,5.738,6.2,5.925,6.2,
0,7.1,6.2,
6.75,2,14,
2,7.705,2.129,7.265,2,7.523,2,
1,7.871,2.295,7.802,2.198,
2,8,3.25,8.001,2.477,8,2.735,
0,8,3.95,
2,7.871,4.905,8,4.465,8,4.723,
3,7.705,5.071,0.7,0.7,0,1,
2,6.75,5.201,7.523,5.201,7.265,5.201,
2,5.795,5.071,6.235,5.201,5.977,5.201,
3,5.629,4.905,0.7,0.7,0,1,
2,5.5,3.95,5.499,4.723,5.5,4.465,
0,5.5,3.25,
2,5.629,2.295,5.5,2.735,5.5,2.477,
3,5.795,2.129,0.7,0.7,0,1,
2,6.75,2,5.977,1.999,6.235,2,
3.6,2,14,
2,4.294,2.096,3.975,2,4.163,2,
3,4.404,2.206,0.5,0.5,0,1,
2,4.5,2.9,4.5,2.337,4.5,2.526,
2,4.404,3.594,4.5,3.275,4.5,3.462,
3,4.294,3.704,0.5,0.5,0,1,
2,3.6,3.8,4.163,3.8,3.974,3.8,
0,2.9,3.8,
2,2.206,3.704,2.525,3.8,2.337,3.8,
3,2.096,3.594,0.5,0.5,0,1,
2,2,2.9,2,3.462,2,3.275,
2,2.096,2.206,2,2.525,2,2.337,
3,2.206,2.096,0.5,0.5,0,1,
2,2.899,2,2.337,2,2.525,2,
0,3.6,2
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
                    if large_arc:
                        if delta_theta >= 0:
                            delta_theta -= 2 * pi
                        else:
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