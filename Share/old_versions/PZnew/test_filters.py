# -*- coding: utf-8 -*-
"""
Created on Wed Mar 11 23:31:58 2020

@author: qo
"""

from scipy.signal import savgol_filter
import numpy as np
import matplotlib.pyplot as plt

data = np.genfromtxt("calibration_trace.txt")

if False:
    fig, ax = plt.subplots(3, 4, figsize=(12,8))
    for i, x in enumerate([51,101,201]):
        for j, y in enumerate([1,3,5,7]):
            filtered = savgol_filter(data, x, y)

            ax[i,j].plot(data)
            ax[i,j].plot(filtered)
            ax[i,j].set_title("%d, %d" % (x, y))
    plt.tight_layout()
    plt.show()

filtered = savgol_filter(data, 101, 7)
MAX = np.max(filtered)
tenpercent = np.argmax(filtered>0.1*MAX)
ninetypercent = np.argmax(filtered>0.9*MAX)
distance = ninetypercent - tenpercent
small_span = [int(tenpercent-distance), int(tenpercent+0.3*distance)]
big_span = [int(tenpercent-distance), int(ninetypercent+distance)]

ZERO = np.mean(filtered[:small_span[0]])
LIMIT = 0.95*MAX
SPAN = LIMIT-ZERO

onepercent = np.argmax(filtered>ZERO+0.01*SPAN)
zeroindex = onepercent
while filtered[zeroindex]>ZERO+0.001*SPAN:
    zeroindex -= 1
pivotx = [zeroindex, onepercent]
for percent in np.linspace(0.02, 1.0, 99):
    index = np.argmax(filtered>ZERO+percent*SPAN)
    if filtered[index]+filtered[index-1] < 2*(ZERO+percent*SPAN):
        index -= 1
    pivotx.append(index)
pivoty = np.linspace(ZERO, ZERO+SPAN, 101)

fig, ax = plt.subplots(1,3,figsize=(12,3))
ax[0].plot(data)
ax[0].plot(filtered)
ax[0].set_xlim(big_span)
ax[1].plot(filtered)
ax[1].plot(data)
ax[1].scatter(pivotx, pivoty)
ax[1].hlines(np.linspace(ZERO, LIMIT, 101), 0, 1000)
ax[1].set_xlim(small_span)
ax[1].set_ylim(ZERO-SPAN/50, ZERO+SPAN/10)
ax[2].scatter(pivotx, pivoty)

plt.show()
