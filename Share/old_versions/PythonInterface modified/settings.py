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
        # Frequency Lock
        self.lock_offset = tk.StringVar(GUI)
        self.lock_flank = tk.StringVar(GUI)
        self.lock_relsens = tk.StringVar(GUI)
        self.lock_piezo = tk.StringVar(GUI)
        self.lock_transm = tk.StringVar(GUI)
        self.lock_errors = tk.StringVar(GUI)
        
        # load settings from file
        self.Load()
        
        # connect callback to stringvars so that the config file is always up to date (note: some settings should only been saved after sending them to the microcontroller)
        for stringvar in [self.name, self.scope_time, self.lin_pivots, self.trace_from, self.trace_to, self.trace_stepsize, self.lock_offset, self.lock_flank, self.lock_relsens, self.lock_piezo, self.lock_transm, self.lock_errors]:
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
        self.lock_offset.set(lines[12].rstrip())
        self.lock_flank.set(lines[13].rstrip())
        self.lock_relsens.set(lines[14].rstrip())
        self.lock_piezo.set(lines[15].rstrip())
        self.lock_transm.set(lines[16].rstrip())
        self.lock_errors.set(lines[17].rstrip())
        
        fil.close()
        
    def Save(self):
        fil = open(filename, 'w')    
        fil.write('\n'.join([self.name.get(), self.ADC_Setting.get(), self.DAC_Setting.get(), self.kp.get(), self.ki.get(), self.kd.get(), self.scope_time.get(), self.lin_pivots.get(), self.lin_timestamp.get(), self.trace_from.get(), self.trace_to.get(), self.trace_stepsize.get(), self.lock_offset.get(), self.lock_flank.get(), self.lock_relsens.get(), self.lock_piezo.get(), self.lock_transm.get(), self.lock_errors.get()]))
        fil.close()
        
    def GetLinPivots(self):
        return np.array(json.loads(self.lin_pivots.get()), dtype=float)
    
    def SetLinPivots(self, array):
        self.lin_pivots.set(json.dumps(array.tolist()))
        
    def GetLockTrace(self):
        return np.array(json.loads(self.lock_piezo.get()), dtype=float), np.array(json.loads(self.lock_transm.get()), dtype=float), np.array(json.loads(self.lock_errors.get()), dtype=float)
    
    def SetLockTrace(self, array):
        self.lock_piezo.set(json.dumps(array.tolist()))
        self.lock_transm.set(json.dumps(array.tolist()))
        self.lock_errors.set(json.dumps(array.tolist()))