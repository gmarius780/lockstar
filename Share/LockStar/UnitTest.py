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


class UnitTest(tk.Tk):
    def __init__(self):
        self.MCU = MCU_Handler(self, speed=20000000)
        self.winfo_toplevel().title("LockStar Self Test")
        self.winfo_toplevel().config(bg='white')
        self.geometry("500x800")
        self.resizable(False, True)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        

UnitTest().mainloop()
