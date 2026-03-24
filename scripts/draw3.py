#!/bin/python

import matplotlib.pyplot as plt
import numpy as np
from tqdm import trange

width = int(input())
height = int(input())

content = []
with open("test2.bin", "rb") as ff:
    bb = ff.read()

    for b in trange(len(bb)):
        content.append(bb[b])

fig, ax = plt.subplots(figsize = (width / 100, height / 100))
ax.imshow(np.array(content).reshape((height, width, 4)))
plt.savefig("/storage/emulated/0/output.png")
