# -*- coding: utf-8 -*-
"""
Created on Fri Mar 27 09:28:18 2020

@author: philip
"""

import tkinter
from tkinter import *
from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

root = tkinter.Tk()

x = np.linspace(0,10,1000)
y = np.sin(2*np.pi*x)
fig, ax = plt.subplots(figsize=(20,2.5))
ax.plot(x, y)

canvas_frame = tkinter.Frame(root, width=400, height=400)
canvas_frame.pack()
canvas_frame.pack_propagate(False)

canvas = FigureCanvasTkAgg(fig, master=canvas_frame)
canvas.draw()
canvas.get_tk_widget().pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=1)

toolbar = NavigationToolbar2Tk(canvas, root)
toolbar.update()
canvas.get_tk_widget().pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=1)

scrollbar = tkinter.Scrollbar(master=root, orient=HORIZONTAL)
scrollbar.pack(side=tkinter.BOTTOM, fill=X)
scrollbar["command"] = canvas.get_tk_widget().xview

tkinter.mainloop()