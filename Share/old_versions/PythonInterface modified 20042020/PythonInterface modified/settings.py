# -*- coding: utf-8 -*-
"""
Created on Tue Mar 10 16:30:09 2020

@author: qo
"""

import tkinter as tk
from macros import *
import numpy as np
import json

filename = "config.txt"

class Settings:
    def __init__(self, GUI):
        # general
        self.name = tk.StringVar(GUI)
        # in/output settings
        self.ADC_Setting = tk.StringVar(GUI)
        self.DAC_Setting = tk.StringVar(GUI)
        # PID
        self.kp = tk.StringVar(GUI)
        self.ki = tk.StringVar(GUI)
        self.kd = tk.StringVar(GUI)
        # view
        self.scope_time = tk.StringVar(GUI)
        # AOM linearization
        self.lin_pivots = tk.StringVar(GUI)
        self.lin_timestamp = tk.StringVar(GUI)
        # Trace Recording
        self.trace_from = tk.StringVar(GUI)
        self.trace_to = tk.StringVar(GUI)
        self.trace_stepsize = tk.StringVar(GUI)
        
        # load settings from file
        self.Load()
        
        # connect callback to stringvars so that the config file is always up to date (note: some settings should only been saved after sending them to the microcontroller)
        for stringvar in [self.name, self.scope_time, self.lin_pivots]:
            stringvar.trace('w', lambda name, index, mode, sv=stringvar: self.Save())
        
    
    def Load(self):
        fil = open(filename, 'r') 
        lines = fil.readlines()
          
        self.name.set(lines[0].rstrip())
        self.ADC_Setting.set(lines[1].rstrip())
        self.DAC_Setting.set(lines[2].rstrip())
        self.kp.set(lines[3].rstrip())
        self.ki.set(lines[4].rstrip())
        self.kd.set(lines[5].rstrip())
        self.scope_time.set(lines[6].rstrip())
        self.lin_pivots.set(lines[7].rstrip())
        self.lin_timestamp.set(lines[8].rstrip())
        self.trace_from.set(lines[9].rstrip())
        self.trace_to.set(lines[10].rstrip())
        self.trace_stepsize.set(lines[11].rstrip())
        
        fil.close()
        
    def Save(self):
        fil = open(filename, 'w')    
        fil.write('\n'.join([self.name.get(), self.ADC_Setting.get(), self.DAC_Setting.get(), self.kp.get(), self.ki.get(), self.kd.get(), self.scope_time.get(), self.lin_pivots.get(), self.lin_timestamp.get(), self.trace_from.get(), self.trace_to.get(), self.trace_stepsize.get()]))
        fil.close()
        
    def GetLinPivots(self):
        return np.array(json.loads(self.lin_pivots.get()), dtype=float)
    
    def SetLinPivots(self, array, timestamp):
        self.lin_timestamp.set(timestamp)
        self.lin_pivots.set(json.dumps(array.tolist()))