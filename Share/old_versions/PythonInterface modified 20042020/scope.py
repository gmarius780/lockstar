# -*- coding: utf-8 -*-
"""
Created on Tue Mar 10 16:53:47 2020

@author: qo
"""

import numpy as np

SCOPE_POINTS = 1000
SCOPE_LOG_CUTOFF = -4
PLOT_RANGES = {'0-5V': [-0.5, 5.5], '0-10V': [-1, 11], '+/- 5V': [-6, 6], '+/- 10V': [-12, 12]}

def logify(data, cutoff=-4):
    for i in range(len(data)):
        if data[i]>0:
            data[i] = max(np.log10(data[i]), cutoff)-cutoff
        elif data[i]<0:
            data[i] = -max(np.log10(-data[i]), cutoff)+cutoff
            
def TwosComplement2Float(codes, float_range):
    print(float_range)
    print(codes)
    for i in range(len(codes)):
        if codes[i] >= 2**19:
            codes[i] -= 2**20
        codes[i] += 2**19
        codes[i] = float_range[0] + codes[i] * (float_range[1] - float_range[0]) / (2**20-1)
    print(codes)
