# -*- coding: utf-8 -*-
"""
Created on Fri Mar 27 09:46:13 2020

@author: philip
"""

import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import numpy as np

class ScrollingPlot(tk.Frame):
    def __init__(self, root, width, height, logging=False):
        self.logging = logging
        
        tk.Frame.__init__(self, root)
        self.fig, self.ax = plt.subplots(figsize=(width,height))
        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH)
        
        # bind events to canvas
        self.canvas.mpl_connect('button_press_event', self.button_press_callback)
        self.canvas.mpl_connect('scroll_event', self.scroll_callback)
        
        self.zoomlevel = 1
        
    def plot(self, x, y1, y2=None, kwargs1={}, kwargs2={}):
        # remove previous plots
        while len(self.ax.lines)>0:
            self.ax.lines.pop()
        self.ll = x[0]
        self.ul = x[-1]
        self.span = x[-1]-x[0]
        self.center = (self.span)/2
        self.ax.plot(x, y1, **kwargs1)
        if not y2 is None:
            self.ax.plot(x, y2, **kwargs2)
        self.redraw()
        
    def redraw(self):
        if self.ul <= self.center + self.span/(2**self.zoomlevel):
            self.ax.set_xlim(self.ul-2*self.span/(2**self.zoomlevel), self.ul)
        elif self.ll >= self.center - self.span/(2**self.zoomlevel):
            self.ax.set_xlim(self.ll, self.ll+2*self.span/(2**self.zoomlevel))
        else:
            self.ax.set_xlim(max(self.ll, self.center - self.span/(2**self.zoomlevel)), min(self.ul,self.center + self.span/(2**self.zoomlevel)))
        self.ax.set_title("{0:.6g} % Zoom (left click to zoom in, right click to zoom out)".format(100.0/(2**(self.zoomlevel-1))))
        self.canvas.draw()
        
    def button_press_callback(self, event):
        # log
        if self.logging:
            print ("clicked with mouse button %d at x=%f" % (event.button, event.xdata))
        # adjust zoom level
        if event.button == 1:
            self.zoomlevel += 1
        if event.button == 3 and self.zoomlevel>1:
            self.zoomlevel -= 1
        # adjust scroll position to clicked position
        self.center = event.xdata
        # redraw figure with new boundaries
        self.redraw()
        
        
    def scroll_callback(self, event):
        # log
        if self.logging:
            if event.button=="up":
                print(">")
            if event.button=="down":
                print("<")
        # shift plotting window
        if event.button=="up":
            self.center += self.span/(2**(self.zoomlevel+2))
        if event.button=="down":
            self.center -= self.span/(2**(self.zoomlevel+2))
        # clip to span
        self.center = np.clip(self.center, self.span/(2**(self.zoomlevel)), self.ul-self.span/(2**(self.zoomlevel)))
        # redraw figure with new boundaries
        self.redraw()
            
if __name__=="__main__":
    root = tk.Tk()
    SP = ScrollingPlot(root, 4, 3)
    SP.plot(np.linspace(0,1,100),np.linspace(0,1,100)**2, kwargs1={'c':'red'})
    SP.pack()
    tk.mainloop()