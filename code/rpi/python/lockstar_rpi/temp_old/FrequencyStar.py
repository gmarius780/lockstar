# -*- coding: utf-8 -*-


# necessary python packages
import tkinter as tk
from collections import deque

# lockstar pacakges
from scope import *
from controlgui import ControlGUI
from macros import *
from plotscan import PlotScan
from spi import *
from settings import Settings

# for trace analysis
import scipy.ndimage as ndi
import numpy as np


def lockpoints_function(voltage, transm, errors, sign_flank, rel_threshold, offset_shift):
    # calculate derivative/gradient
    grad = errors[1:]-errors[:-1]
    grad = ndi.gaussian_filter1d(grad, 25)
    
    # get steepest gradient
    strongest = np.max(grad*sign_flank)
    threshold = rel_threshold * strongest
    
    # Identify gradients that are relatively steep, from there move left and right until an extremum is reached.
    # This marks the capture range of the lock.
    flanks = []
    grad_copy = grad*sign_flank
    while(np.max(grad_copy)>=threshold):
        # find biggest flank remaining
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
        # save flank
        flanks.append([ll,ul])
        # remove flank from data
        grad_copy[ll:ul] = -1
        
    # Now get the lockpoints for each flank.
    lockpoints = []
    for flank in flanks:
        # define
        offset = np.mean(errors[flank[0]:flank[1]])+offset_shift
        center = voltage[flank[0]+np.argmin(np.abs(errors[flank[0]:flank[1]]-offset))]
        lockpoints.append([center, offset])
    
    extra = None
    
    return lockpoints, flanks, extra

def waypoints_function(voltage, transm, errors, lockpoints, flanks, extra):
    waypoints = deque()
    return waypoints




class FrequencyStar(ControlGUI):
    def __init__(self):
        ControlGUI.__init__(self, name="FREQUENCYSTAR")
        self.add_scan_box(x=50, y=80, lockpoints_function=lockpoints_function, waypoints_function=waypoints_function)
        self.add_pid_box(x=50, y=470)
        self.add_io_box(x=300, y=470)
        self.add_live_box(x=600,y=470, label1="Transm. (V)", label2="Error (V)")
        

FrequencyStar().mainloop()
