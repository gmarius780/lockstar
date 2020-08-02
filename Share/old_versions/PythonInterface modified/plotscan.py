# -*- coding: utf-8 -*-
"""
Created on Fri Mar 27 09:46:13 2020

@author: philip
"""

import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
import time

class PlotScan(tk.Frame):
    def __init__(self, root, width, height, logging=False):
        self.logging = logging
        
        tk.Frame.__init__(self, root)
        self.fig = plt.figure(figsize=(width,height))
        self.ax1 = self.fig.add_axes([0.12,0.54,0.83,0.38])
        self.ax2 = self.fig.add_axes([0.12,0.12,0.83,0.38])
        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH)
        
        # bind events to canvas
        self.canvas.mpl_connect('button_press_event', self.button_press_callback)
        self.canvas.mpl_connect('scroll_event', self.scroll_callback)
        
        self.zoomlevel = 1
        
        self.ax2.set_xlabel("Control Voltage (V)")
        self.ax1.set_ylabel("Transm. (V)")
        self.ax2.set_ylabel("Error  (V)")
        
    def plot(self, x, y1, y2=None, kwargs1={}, kwargs2={}):
        self.remove_plots()

        self.ll = x[0]
        self.ul = x[-1]
        self.span = x[-1]-x[0]
        self.center = (self.span)/2
        # adjust y range
        y1r = np.max(y1)-np.min(y1)
        y2r = np.max(y2)-np.min(y2)
        self.y1lim = [np.min(y1)-0.1*y1r, np.max(y1)+0.1*y1r]
        self.y2lim = [np.min(y2)-0.1*y2r, np.max(y2)+0.1*y2r]
        self.ax1.set_ylim(self.y1lim)
        self.ax2.set_ylim(self.y2lim)
        
        self.ax1.plot(x, y1, **kwargs1)
        self.ax2.plot(x, y2, **kwargs2)
        
        # reset zoom
        self.zoomlevel = 1
        
        self.marks = []
        self.marked = 0
        
        self.redraw()
    
        
    def redraw(self):
        if self.ul <= self.center + self.span/(2**self.zoomlevel):
            self.ax1.set_xlim(self.ul-2*self.span/(2**self.zoomlevel), self.ul)
            self.ax2.set_xlim(self.ul-2*self.span/(2**self.zoomlevel), self.ul)
        elif self.ll >= self.center - self.span/(2**self.zoomlevel):
            self.ax1.set_xlim(self.ll, self.ll+2*self.span/(2**self.zoomlevel))
            self.ax2.set_xlim(self.ll, self.ll+2*self.span/(2**self.zoomlevel))
        else:
            self.ax1.set_xlim(max(self.ll, self.center - self.span/(2**self.zoomlevel)), min(self.ul,self.center + self.span/(2**self.zoomlevel)))
            self.ax2.set_xlim(max(self.ll, self.center - self.span/(2**self.zoomlevel)), min(self.ul,self.center + self.span/(2**self.zoomlevel)))
        self.ax1.set_xticklabels([])
        self.ax1.set_title("{0:.6g} % Zoom (left click to zoom in, right click to zoom out)".format(100.0/(2**(self.zoomlevel-1))), fontsize=10)
        self.canvas.draw()
        
    def button_press_callback(self, event):
        for i,mark in enumerate(self.marks):
            click_pos = self.ax2.transData.transform([event.xdata,event.ydata])
            marked_pos = self.ax2.transData.transform(mark)
            dist = np.linalg.norm(click_pos-marked_pos)
            if dist<10:
                self.marked = i
                self.plot_marks()
                return

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
        
    def remove_plots(self):
        # remove previous plots
        while len(self.ax1.lines)>0:
            self.ax1.lines.pop()
        while len(self.ax2.lines)>0:
            self.ax2.lines.pop()
        
    def remove_lines(self):
        i = 0
        while i < len(self.ax2.collections):
            if type(self.ax2.collections[i]) is mpl.collections.LineCollection:
                del self.ax2.collections[i]
            else:
                i += 1
                
    def remove_points(self):
        i = 0
        while i < len(self.ax2.collections):
            if type(self.ax2.collections[i]) is mpl.collections.PathCollection:
                del self.ax2.collections[i]
            else:
                i += 1
                
    def remove_filling(self):
        for ax in [self.ax1, self.ax2]:
            i = 0
            while i < len(ax.collections):
                if type(ax.collections[i]) is mpl.collections.PolyCollection:
                    del ax.collections[i]
                else:
                    i += 1
        
    def hline(self, pos):
        self.remove_lines()
        self.ax2.hlines(pos, self.ll, self.ul, color='C2', linestyle=':')
        self.canvas.draw()
        
    def mark(self, lockpoints):
        self.marks = lockpoints
        
    def plot_marks(self):
        self.remove_points()
        print(len(self.marks))
        print(self.marked)
        time.sleep(0.1)
        c = ['C2']*len(self.marks)
        c[self.marked] = 'r'
        self.ax2.scatter(self.marks[:,0], self.marks[:,1], c=c, s=100, alpha=0.9)
        self.canvas.draw()
        
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
        self.center = np.clip(self.center, self.ll+self.span/(2**(self.zoomlevel)), self.ul-self.span/(2**(self.zoomlevel)))
        # redraw figure with new boundaries
        self.redraw()
        
    def plot_lock_points(self, piezo, flanks, lockpoints):
        self.remove_filling()
        for flank in flanks:
            self.ax1.fill_between([piezo[flank[0]],piezo[flank[1]]], self.y1lim[0], self.y1lim[1], fc='#dddddd', alpha=0.4, lw=0.7, ec='k', zorder=-20)
            self.ax2.fill_between([piezo[flank[0]],piezo[flank[1]]], self.y2lim[0], self.y2lim[1], fc='#dddddd', alpha=0.4, lw=0.7, ec='k', zorder=-20)
            
        self.plot_marks()
            
if __name__=="__main__":
    root = tk.Tk()
    SP = PlotScan(root, 4, 3)
    SP.plot(np.linspace(0,1,100),np.linspace(0,1,100)**2, np.linspace(0,1,100)**4, kwargs1={'c':'red'}, kwargs2={'c':'black'})
    SP.pack()
    tk.mainloop()
