# -*- coding: utf-8 -*-
"""
Created on Tue Apr 21 22:04:11 2020

@author: philip
"""

import matplotlib.pyplot as plt
import numpy as np
import scipy
from scipy.signal import savgol_filter
import scipy.ndimage as nd
import copy
import time

FLANK_FALLING = -1
FLANK_RISING = +1

# for noise cancellation: smoothen array
def smoothen(arr, rng):
    result = np.zeros(len(arr))
    for n in range(0,len(arr)):
        ll = max(0, n-rng)
        ul = min(len(arr)-1, n+rng)+1
        tmp = 0.0
        for val in arr[ll:ul]:
            tmp += val
        result[n] = tmp / float(ul-ll)
    return result


def get_lock_points(piezo, transm, errors, offset, signal_threshold="AUTO", flank=FLANK_FALLING, smooth_window=11):
    
    # apply smoothing filter
    errors = smoothen(errors, smooth_window)

    # set offset
    offset = 0.00
    
    # set minimum size of signal
    if signal_threshold=="AUTO":
        signal_threshold = 0.2 * (np.max(errors)-np.min(errors))
    
    # iterate through array and identify "zero crossings"
    prev_grad = 0
    last_extremum = errors[0]
    lines = []
    crossing = None
    for i in np.arange(1,len(errors)):
        grad = errors[i]-errors[i-1]
        # did we cross the offset line?
        if (errors[i-1]-offset)*np.sign(grad)<0 and (errors[i]-offset)*np.sign(grad)>0:
            crossing = i
        # did the direction change?
        if grad*prev_grad<0:
            new_extremum = errors[i-1]
            # correct flank?
            if prev_grad*flank>0 and not crossing is None:
                extremum_span = np.abs(new_extremum-last_extremum)
                if extremum_span>signal_threshold:
                    lines.append((piezo[crossing]+piezo[crossing-1])/2)
            crossing = None
            last_extremum = new_extremum
        prev_grad = grad
        
    return lines

def Gaussian_filter(data, win_size):
    ws2 = win_size//2
    result = np.zeros(len(data))
    kernel = np.exp(-2*(np.arange(win_size)-ws2)**2 / (ws2**2))
    for i in range(len(result)):
        ll = min(i,ws2)
        ul = min(ws2, len(data)-i-1)
        result[i] = np.dot(data[i-ll:i+ul], kernel[ws2-ll:ws2+ul]) / np.sum(kernel[ws2-ll:ws2+ul])
    return result

def get_flanks(piezo, errors, rel_threshold=0.1, flank=FLANK_FALLING, smooth_window=51):
    
    grad = errors[1:]-errors[:-1]
    start = time.time()
    grad = nd.gaussian_filter1d(grad, smooth_window/2)# Gaussian_filter(grad, smooth_window)
    end = time.time()
    print("Gaussian filter: %f" % (end-start))
    
    strongest = np.max(grad*flank)
    threshold = rel_threshold * strongest
    
    
    flanks = []
    grad_copy = grad*flank
    while(np.max(grad_copy)>=threshold):
        # find biggest flank
        max_pos = np.argmax(grad_copy)
        # find left edge of flank
        i = max_pos
        while i>0 and grad_copy[i]>0:
            i -= 1
        ll = i + 1
        # find right edge of flank
        i = max_pos
        while i<len(grad_copy) and grad_copy[i]>0:
            i += 1
        ul = i - 1
        # abort when intervals become zero
        if ul==ll:
            break
        # remove flank from data
        grad_copy[ll:ul] = -1
        
        flanks.append([ll,ul])
    
    return np.array(flanks)
    
def get_lock_points_from_flanks(piezo, errors, flanks, offset_shift=0):          
    lockpoints = []
    for flank in flanks:
        offset = np.mean(errors[flank[0]:flank[1]])+offset_shift
        center = piezo[flank[0]+np.argmin(np.abs(errors[flank[0]:flank[1]]-offset))]
        lockpoints.append([center, offset])
    return np.array(lockpoints)
    
def plot_everything(piezo, transm, errors, flanks, lockpoints):
    e_max = np.max(errors)
    e_min = np.min(errors)
    e_span = e_max - e_min
    t_max = np.max(transm)
    t_min = np.min(transm)
    t_span = t_max - t_min
    
    fig, [ax, ax2] = plt.subplots(2,1)

    ax2.plot(piezo, errors)
     
    for flank in flanks:
        ax.fill_between([piezo[flank[0]],piezo[flank[1]]], t_min-0.1*t_span, t_max+0.1*t_span, fc='#dddddd', alpha=0.4, lw=0.7, ec='k')
        ax2.fill_between([piezo[flank[0]],piezo[flank[1]]], e_min-0.1*e_span, e_max+0.1*e_span, fc='#dddddd', alpha=0.4, lw=0.7, ec='k')
        
    ax2.scatter(lockpoints[:,0], lockpoints[:,1], c='k', s=20, zorder=20)
    
    ax.plot(piezo, transm, c='C1', alpha=1, zorder=20)
    
    ax.set_ylim(t_min-0.1*t_span, t_max+0.1*t_span)
    ax2.set_ylim(e_min-0.1*e_span, e_max+0.1*e_span)
    plt.show()
    
    
LARGER = 1
SMALLER = -1
def generate_waypoints(piezo, errors, start, stop, flank, tolerance, flank_width):
    if start==stop:
        return []
    inv_moving_direction = np.sign(start-stop)
    start_index = np.argmax(piezo>=stop)
    end_index = np.argmax(piezo>=start)
    index = start_index
    last_level = errors[start_index]
    current_grad = flank
    
    # first waypoint is lock point itself, next waypoint from first extremum
    fro = start_index - (flank_width if inv_moving_direction<0 else 0)
    to = fro + flank_width
    if flank*inv_moving_direction>0:
        waypoints = [[SMALLER, errors[start_index]], [LARGER, -tolerance+np.max(errors[fro:to])]]
    else:
        waypoints = [[LARGER, errors[start_index]], [SMALLER, tolerance+np.min(errors[fro:to])]]
    index += int(inv_moving_direction*flank_width)
    
    while not np.abs(waypoints[-1][1]) > np.max(waypoints[-1][0]*errors[min(index,end_index):max(index,end_index)]):
        unique_up_to = np.argmin(np.abs(waypoints[-1][1]) > waypoints[-1][0]*errors[min(index,end_index):max(index,end_index)]) + min(index,end_index)
        waypoints.append([-waypoints[-1][0], (-waypoints[-1][0])*(np.max((-waypoints[-1][0])*errors[min(index,unique_up_to):max(index,unique_up_to)])-tolerance)])
        index = unique_up_to
    
    return waypoints[::-1]

if __name__ == "__main__":

    FLANK_FALLING = -1
    FLANK_RISING = +1

    start = time.time()
    data = np.genfromtxt("detailedtrace.txt")
    end = time.time()
    print("loading: %f" % (end-start))
    clip = 0
    piezo = np.linspace(-200,200,1000000)
    transm = data[1,clip:]
    errors = data[0,clip:]
    start = time.time()
    flanks = get_flanks(piezo, errors, rel_threshold=0.35, flank=FLANK_RISING)
    end = time.time()
    print("flank: %f" % (end-start))
    start = time.time()
    lockpoints = get_lock_points_from_flanks(piezo, errors, flanks, 0.0)
    end = time.time()
    print("lockpoints: %f" % (end-start))
    start = time.time()
    plot_everything(piezo, transm, errors, flanks, lockpoints)
    end = time.time()
    print("plot: %f" % (end-start))
