#!/bin/python3
import turtle as t

pts = [a.replace("\n", "").split(",") for a in open("out.csv").readlines()]
t.speed(1000000)
t.penup()
for a in pts:
    if a[0] == "-1" or a[0] == "-2":
        t.penup()
    else:
        t.pendown()
        t.goto(float(a[0]) - 100, float(a[1]) - 100)
t.mainloop()