# -*- coding: utf-8 -*-
"""
Created on Wed Feb 26 14:35:47 2020

@author: https://www.quora.com/How-do-I-create-a-real-time-plot-with-matplotlib-and-Tkinter
"""

from tkinter import *
from random import randint
 
# these two imports are important
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import time
import threading
import numpy as np
 
continuePlotting = False
 
def change_state():
    global continuePlotting
    if continuePlotting == True:
        continuePlotting = False
    else:
        continuePlotting = True
    
 
def app():
    # initialise a window.
    root = Tk()
    root.config(background='white')
    root.geometry("1000x700")
    
    lab = Label(root, text="Live Plotting", bg = 'white').pack()
    
    fig = Figure()
    
    ax = fig.add_subplot(111)
    ax.set_xlabel("X axis")
    ax.set_ylabel("Y axis")
    ax.grid()
    ax.set_xlim(0,10)
    ax.set_ylim(-1,11)
    
    graph = FigureCanvasTkAgg(fig, master=root)
    graph.get_tk_widget().pack(side="top",fill='both',expand=True)
 
    def plotter():
        # 19.6 fps on 10 points
        # 18.5 fps for 100 points
        # 13.3 fps for 1000 points
        n = 1000
        xr = np.linspace(0,10,n)
        
        while continuePlotting:
            t1 = time.time()
            for j in range(100):
                if len(ax.lines)>0:
                    ax.lines.pop(0)
                dpts = [randint(0, 10) for i in range(n)]#data_points()
                ax.plot(xr, dpts, marker='o', color='orange')
                graph.draw()
            t2 = time.time()
            print (100.0/(t2-t1))
 
    def gui_handler():
        change_state()
        plotter()
        #threading.Thread(target=plotter).start()
 
    b = Button(root, text="Start/Stop", command=gui_handler, bg="red", fg="white")
    b.pack()
    
    root.mainloop()
 
if __name__ == '__main__':
    app()
