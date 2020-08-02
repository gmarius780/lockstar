# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

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


class PowerStar(ControlGUI):
    def __init__(self):
        ControlGUI.__init__(self, name="POWERSTAR")
        self.add_live_box(x=50,y=100, label1="Set Value (V)", label2="Is Value (V)")
        self.add_pid_box(x=50, y=470)
        self.add_io_box(x=300, y=470)
        self.add_cal_box(x=600, y=470)
        

PowerStar().mainloop()
