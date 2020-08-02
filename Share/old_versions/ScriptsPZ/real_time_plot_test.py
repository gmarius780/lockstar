# -*- coding: utf-8 -*-
"""
Created on Wed Feb 26 14:35:47 2020

@author: https://www.quora.com/How-do-I-create-a-real-time-plot-with-matplotlib-and-Tkinter
"""

from tkinter import *
from random import randint
import RPi.GPIO as GPIO
from collections import deque
from new_spi import *
 
# these two imports are important
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import time
import threading
import numpy as np

GPIO.setmode(GPIO.BCM)
input_channel = 8
GPIO.setup(input_channel, GPIO.IN)
NSamples = 1000



 
def app():
    # initialise a window.
    root = Tk()
    root.config(background='white')
    root.geometry("1000x700")
    read_queue = deque()
    write_queue = deque()
    
    spi = SPI_Handler()
    
    lab = Label(root, text="Live Plotting", bg = 'white').pack()
    
    fig = Figure()
    
    ax = fig.add_subplot(111)
    ax.set_xlabel("X axis")
    ax.set_ylabel("Y axis")
    ax.grid()
    ax.set_xlim(0,1)
    ax.set_ylim(-11,11)
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Voltage (V)")
    xr = np.linspace(0,1,NSamples)
    
    graph = FigureCanvasTkAgg(fig, master=root)
    graph.get_tk_widget().pack(side="top",fill='both',expand=True)
 
    def plotter():
        while True:
            # wait for input pin to go high
            while not GPIO.input(input_channel):
                pass
            # read data
            data = ReadFloatChunk(spi, 1000)
            while len(ax.lines)>0:
                ax.lines.pop(0)
            ax.plot(xr, data, c='red')
            graph.draw()
            # wait for input pin to go low
            while GPIO.input(input_channel):
                pass
                
    def getData():
        while True:
            # wait for input pin to go high
            while not GPIO.input(input_channel):
                pass
            # read data
            if len(write_queue)>0:
                data = spi.transfer(write_queue.popleft())
            else:
                data = spi.transfer()
            print("Got data!")
            read_queue.append(data)
            # wait for input pin to go low
            while GPIO.input(input_channel):
                pass
 
    def send_cmd():
        print(w.get())
        DP = DataPackage()
        DP.addUInt8(17)
        DP.addUInt16(int(w.get()))
        write_queue.append(DP)
        
    def send_pid():
        DP = DataPackage()
        DP.addUInt8(18)
        DP.addFloat(PID_p.get())
        DP.addFloat(PID_i.get())
        DP.addFloat(PID_d.get())
        write_queue.append(DP)
        
    def check_queue():
        if len(read_queue)>0:
            while len(ax.lines)>0:
                ax.lines.pop(0)
            data = read_queue.popleft()
            ax.plot(xr, data, c='red')
            graph.draw()
        root.after(100, check_queue)
 
    w = Spinbox(root, from_=1, to=200)
    w.pack()
    b = Button(root, text="Send", command=send_cmd, bg="red", fg="white")
    b.pack()
    
    PID_p = Spinbox(root, from_=0.0000, to=1.0000)
    PID_p.pack()
    PID_i = Spinbox(root, from_=0.0000, to=1.0000)
    PID_i.pack()
    PID_d = Spinbox(root, from_=0.0000, to=1.0000)
    PID_d.pack()
    PID_button = Button(root, text="Send", command=send_pid, bg="red", fg="white")
    PID_button.pack()
    
    threading.Thread(target=getData).start()
    root.after(100, check_queue)
    root.mainloop()
 
app()
