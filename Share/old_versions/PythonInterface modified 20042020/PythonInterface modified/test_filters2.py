# -*- coding: utf-8 -*-
"""
Created on Wed Mar 11 23:31:58 2020

@author: qo
"""

from scipy.signal import savgol_filter
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from scipy.interpolate import interp1d
from scipy.optimize import root_scalar
import matplotlib.ticker as mtick

mpl.style.use('dark_background')

data = np.genfromtxt("calibration_trace.txt")
AOM_voltage = np.linspace(0,5,1000)

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
LIMIT = 0.97*MAX
SPAN = LIMIT-ZERO

interpolation = interp1d(AOM_voltage, filtered)

pivots_voltage = [root_scalar(lambda x: (interpolation(x)-ZERO)/SPAN-fraction, bracket=[AOM_voltage[big_span[0]], AOM_voltage[big_span[1]]]).root for fraction in [0.001]+list(np.linspace(0.01, 1, 100))]

#zeroindex = onepercent
#while filtered[zeroindex]>ZERO+0.001*SPAN:
#    zeroindex -= 1
#pivotx = [zeroindex, onepercent]
#for percent in np.linspace(0.02, 1.0, 99):
#    index = np.argmax(filtered>ZERO+percent*SPAN)
#    if filtered[index]+filtered[index-1] < 2*(ZERO+percent*SPAN):
#        index -= 1
#    pivotx.append(index)
#pivoty = np.linspace(ZERO, ZERO+SPAN, 101)
#
fig, ax = plt.subplots(1,3,figsize=(12,3))
ax[0].plot(AOM_voltage, data)
ax[0].plot(AOM_voltage, filtered)
#ax[0].hlines([ZERO, ZERO+SPAN], AOM_voltage[big_span[0]], AOM_voltage[big_span[1]], lw=0.5, color='w')
#ax[0].vlines([pivots_voltage[0], pivots_voltage[-1]], ZERO, ZERO+SPAN, lw=0.5, color='w')
ax[0].plot([pivots_voltage[0], pivots_voltage[0], pivots_voltage[-1], pivots_voltage[-1], pivots_voltage[0]], [ZERO, ZERO+SPAN, ZERO+SPAN, ZERO, ZERO], c='C2', lw=0.5)
ax[0].text(pivots_voltage[0]+0.05*(pivots_voltage[-1]-pivots_voltage[0]), ZERO+SPAN*0.95, "ROI", c='C2', va='top', ha='left')
ax[0].set_xlim(AOM_voltage[big_span])
ax[1].plot(AOM_voltage, filtered)
ax[1].plot(AOM_voltage, data)
#ax[1].vlines(pivots_voltage[:8], 0, MAX, lw=0.5)
ax[1].scatter(pivots_voltage, np.linspace(0, 1, 101)*SPAN+ZERO, c='C3', zorder=20)
ax[1].hlines(np.linspace(ZERO, LIMIT, 101), 0, 1000, lw=0.5, color='w')
ax[1].set_xlim(AOM_voltage[small_span[0]], pivots_voltage[12])
ax[1].set_ylim(ZERO-SPAN/100, ZERO+SPAN/10)
ax[2].scatter(pivots_voltage, np.linspace(0, 1, 101)*SPAN+ZERO, c='C3')
#ax[2].hlines([ZERO, ZERO+SPAN], 0, 1000, lw=0.5, color='w')

axt = [None]*3
for i in range(3):
    ax[i].set_xlabel("AOM Input Voltage")
    axt[i] = ax[i].twinx()
    axt[i].set_ylim(100*(ax[i].get_ylim()-ZERO)/SPAN)
    axt[i].yaxis.set_major_formatter(mtick.PercentFormatter())
ax[0].set_ylabel("Photo Diode Voltage")

plt.tight_layout()
plt.show()

pivots_voltage = np.array(pivots_voltage)
pivots_code = [int(code-2**19) if code >= 2**19 else int(code+2**19) for code in pivots_voltage * (2**20-1) / 5.0]

plt.plot(pivots_code, np.linspace(0,1,101))
plt.show()
