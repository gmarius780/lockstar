import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import matplotlib as mpl
import time
import datetime
import numpy as np
from scipy.signal import savgol_filter
from scipy.interpolate import interp1d
from scipy.optimize import root
import matplotlib.ticker as mtick
import time

from spi import *
from macros import *

fontsize = 12

class AutoPIDWindow(tk.Toplevel):
    
    def __init__(self, root):        
        # set up window
        tk.Toplevel.__init__(self)
        self.root = root
        # force focus on this window
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.close)
        
        self.kp = tk.StringVar()
        self.ki = tk.StringVar()
        self.kd = tk.StringVar()

        DP = DataPackage()
        DP.addUInt8(Command_OptimizePID)
        DP.addUInt8(2) # read ADC channel 2
        self.root.parent.MCU.append_write_queue(DP)
        # stop real-time feed
        self.root.parent.MCU.stop_live_feed()
        
        
        # set up parameters
        self.min = tk.StringVar()
        self.max = tk.StringVar()
        self.max.set("1.0")
        self.freq = tk.StringVar()
        
        self.setup_window()
        
    def make_setpoints(self):
        # make set points
        MAX = float(self.max.get())
        MIN = float(self.min.get())
        period = int(2e5/float(self.freq.get()))
        print(float(self.freq.get()))
        print(period)
        self.time = np.linspace(0,5,1000)
        self.setpoints = MIN+0.5*(MAX-MIN)*(1+np.cos(2*np.pi*self.time*200/period))
        
    def optimize(self):
        self.make_setpoints()
        
        lowest_error = np.inf
        ki = 0.0
        kd = 0.0
        for kp in 10**np.linspace(-3, 0, 30):
            print("do %f now" % kp)
            self.data = self.try_setting(kp, ki, kd)
        
            self.plot()
        
            # evaluate
            abs_error = np.sqrt(np.sum((self.setpoints-self.data)**2)/1000)
            print("mean error is %f" % abs_error)
            
            if abs_error < lowest_error:
                self.kp.set(kp)
                self.ki.set(ki)
                self.kd.set(kd)
                lowest_error = abs_error
                print("new best: %f, %f, %f" % (kp, ki, kd))
            
        print("done optimizing")
        
    def try_setting(self, kp, ki, kd):
        # wait for ready flag from MCU
        while not self.root.parent.MCU.get_GPIO_pin():
            pass
        # make data package
        DP = DataPackage()
        DP.addUInt8(0)
        DP.addFloat(kp)
        DP.addFloat(ki)
        DP.addFloat(kd)
        for setpoint in self.setpoints:
            DP.addFloat(setpoint)
        # send data to MCU
        self.root.parent.MCU.transfer(output=DP, sendbytes=(13+4*len(self.setpoints)), readfloats=0)
        # wait for ready flag from MCU
        while self.root.parent.MCU.get_GPIO_pin():
            pass
        while not self.root.parent.MCU.get_GPIO_pin():
            pass
        self.root.parent.MCU.transfer(sendbytes=0, readfloats=1000)
        
        return self.root.parent.MCU.pop_read_queue()
        
        
    def plot(self):
        # clear plot
        while len(self.ax.lines)>0:
            self.ax.lines.pop(0)
        # plot
        self.ax.plot(self.time, self.setpoints, c='C0')
        self.ax.plot(self.time, self.data, c='C1')
        self.redraw()
        
    def redraw(self):
        
        self.graph.draw()
        
    def demo(self):
        self.make_setpoints()
        print("demonstrate %f, %f, %f" % (float(self.kp.get()), float(self.ki.get()), float(self.kd.get())))
        self.data = self.try_setting(float(self.kp.get()), float(self.ki.get()), float(self.kd.get()))
        self.plot()
        
        
    def setup_window(self):
        self.winfo_toplevel().title("Optimize PID Parameters")
        self.winfo_toplevel().config(bg='white')
        frame = tk.Frame(self, background='black')
        frame.pack(padx=3, pady=3)
        #self.geometry("700x400")
        self.resizable(False, False)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        
        # signal setup
        signalframe = tk.Frame(frame)
        signalframe.pack()
        tk.Label(signalframe, text="Signal Min: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(signalframe, from_=0.00, to=10.00, increment=1, borderwidth=0, textvar=self.min, width=4, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        tk.Label(signalframe, text=" Signal Max: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(signalframe, from_=0, to=10.00, increment=1, borderwidth=0, textvar=self.max, width=4, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        tk.Label(signalframe, text=" Signal Frequency (Hz): ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(signalframe, from_=1000, to=20000, increment=1, borderwidth=0, textvar=self.freq, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        
        
        # parameter frame
        parameterframe = tk.Frame(frame)
        parameterframe.pack()
        tk.Label(parameterframe, text="kp: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(parameterframe, from_=0.00, to=10.00, increment=0.01, borderwidth=0, textvar=self.kp, width=4, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        tk.Label(parameterframe, text=" ki: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(parameterframe, from_=0, to=10.00, increment=0.01, borderwidth=0, textvar=self.ki, width=4, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        tk.Label(parameterframe, text=" kd: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(parameterframe, from_=0.00, to=10.00, increment=0.01, borderwidth=0, textvar=self.kd, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        
        tk.Button(parameterframe, text="Optimize", command=self.optimize, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Button(parameterframe, text="Demonstrate", command=self.demo, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)

        # figure
        fig, self.ax = plt.subplots(1,1,figsize=(9,3))
        # ax 1: inputs
        self.ax.set_xlabel("Time (ms)")
        self.ax.set_ylabel("Voltage")
        plt.tight_layout()
        self.graph = FigureCanvasTkAgg(fig, master=frame)
        self.graph.get_tk_widget().pack()
        
        # cancel / accept 
        buttonframe = tk.Frame(frame)
        buttonframe.pack()
        tk.Button(buttonframe, text="Cancel", command=self.close, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Button(buttonframe, text="Accept", command=self.accept, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.RIGHT)
        
        self.redraw()
        
    def close(self):
        # send termination signal to MCU subroutine
        DP = DataPackage()
        DP.addUInt8(2)
        DP.addFloat(0.0)
        DP.addFloat(0.0)
        DP.addUInt32(10)
        DP.addFloat(0.0)
        DP.addFloat(0.0)
        DP.addFloat(0.0)
        self.root.parent.MCU.transfer(output=DP, sendbytes=4013, readfloats=0)
        # wait for ready flag from MCU
        while self.root.parent.MCU.get_GPIO_pin():
            pass
        # restart live stream
        self.root.parent.MCU.mode = 'run'
        # close window
        self.destroy()
        
    def accept(self):
        # send accept signal to MCU subroutine
        DP = DataPackage()
        DP.addUInt8(1)
        DP.addFloat(0.0)
        DP.addFloat(0.0)
        DP.addUInt32(10)
        DP.addFloat(float(self.kp.get()))
        DP.addFloat(float(self.ki.get()))
        DP.addFloat(float(self.kd.get()))
        self.root.parent.MCU.transfer(output=DP, sendbytes=4013, readfloats=0)
        # save settings
        self.root.parent.Config.kp.set(self.kp.get())
        self.root.parent.Config.ki.set(self.ki.get())
        self.root.parent.Config.kd.set(self.kd.get())
        # wait for ready flag from MCU
        while self.root.parent.MCU.get_GPIO_pin():
            pass
        # restart live stream
        self.root.parent.MCU.mode = 'run'
        # close window
        self.destroy()
        
