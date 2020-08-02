# -*- coding: utf-8 -*-
"""
Created on Wed Mar 11 23:31:58 2020

@author: qo
"""

from scipy.signal import savgol_filter
import numpy as np
import matplotlib.pyplot as plt

data = np.array([[np.tan(y-np.pi/2)/10.0+1.8 , y] for y in np.linspace(0.1, np.pi-0.1, 1000)])
noisy_data = data[:,1] + np.random.normal(0, 0.3, 1000)

plt.plot(data[:,0], noisy_data)
plt.show()

filtered = savgol_filter(noisy_data, 501, 1)
plt.semilogy(data[:,0], (filtered-data[:,1]))
plt.show()

mean_errs = np.zeros((6,3))
max_errs = np.zeros((6,3))
for i,x in enumerate([31, 51, 71, 101, 201, 501]):
    for j,y in enumerate([1, 3, 5]):
        mean_err = []
        max_err = []
        for n in range(100):
            noisy_data = data[:,1] + np.random.normal(0, 0.03, 1000)
            filtered = savgol_filter(noisy_data, x, y)
            mean_err.append(np.mean(np.abs(filtered-data[:,1]) / data[:,1]))
            max_err.append(np.max(np.abs(filtered-data[:,1]) / data[:,1]))
        mean_errs[i,j] = np.mean(mean_err)
        max_errs[i,j] = np.mean(max_err)
        
print(mean_errs)
print(max_errs)