#!/bin/python

import matplotlib.pyplot as plt
import numpy as np

ff = open("test2.bin", "rb")
bb = ff.read()
ff.close()

content = []
for b in bb:
    content.append(b)

plt.imshow(np.array(content).reshape((3804, 4096, 4)))
plt.show()
