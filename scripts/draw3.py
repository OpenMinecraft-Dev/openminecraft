#!/bin/python

import matplotlib.pyplot as plt
import numpy as np
from tqdm import trange

content = []
with open("test2.bin", "rb") as ff:
    bb = ff.read()

    for b in trange(len(bb)):
        content.append(bb[b])

plt.imshow(np.array(content).reshape((4567, 3000, 4)))
plt.show()
