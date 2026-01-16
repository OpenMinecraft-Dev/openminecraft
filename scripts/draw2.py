#!/usr/bin/env python3

import turtle

CHAR_IDX = int(input("Unicode id: "))
SCALE = int(input("Scale: "))

vertices = [[float(j) for j in i.replace("\n", "").split(" ")] for i in open(f"out-{CHAR_IDX}.vtx").readlines()]
indices = [int(i.replace("\n", "")) for i in open(f"out-{CHAR_IDX}.idx").readlines()]

indices = [indices[i:i+3] for i in range(0, len(indices), 3)]

turtle.penup()
for (a, b, c) in indices:
    turtle.goto(vertices[a][0] * SCALE, vertices[a][1] * SCALE)
    turtle.pendown()
    turtle.begin_fill()
    turtle.goto(vertices[b][0] * SCALE, vertices[b][1] * SCALE)
    turtle.goto(vertices[c][0] * SCALE, vertices[c][1] * SCALE)
    turtle.end_fill()
    turtle.penup()

turtle.mainloop()
