# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

import tkinter as tk
#from twisted.internet import tksupport, reactor
from functools import partial
from PIL import Image, ImageTk
import numpy as np
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import matplotlib as mpl
from collections import deque
import datetime

from linearize import LinearizeWindow
from scope import *
from macros import *
from spi import *
from settings import Settings

logging = True

class Host:
    def __init__(self, name, ip):
        self.name = name
        self.ip = ip
        self.status = "inactive"
        
    def connect(self):
        self.status = 'active'
    
    def show(self):
        pass


class ControlGUI(tk.Tk):
    def __init__(self, name='Powerstar'):
        tk.Tk.__init__(self, name=name)
        
        self.fontsize = 12
        
        # set up and load configuration (PID parameters, I/O settings, window properties...)
        self.Config = Settings(self)
        
        # makae GUI
        self.setup_window()
        self.setup_plots()
        self.setup_pid_box()
        self.setup_io_box()
        self.setup_lin_box()
        
        # set up SPI to microcontroller and read/write queue
        self.MCU = MCU_Handler(self, verbose=False)
        # make sure all MCU threads stop when window is closed
        self.protocol("WM_DELETE_WINDOW", self.close)
        
    def close(self):
        self.MCU.stop_event.set()
        self.destroy()

    def changeName(self):
        self.Config.name.set(tk.simpledialog.askstring("Change Name", "Enter new name:", initialvalue=self.namewidget.cget("text")))
        #self.namewidget.config(text = self.Config.name.get())
        
        
    def changeScopeTime(self, time):
        # make changes to GUI
        self.ax1.set_xlim(0, time)
        self.ax2.set_xlim(0, time)
        self.plotrange = np.linspace(0, time, int(READ_FLOATS/2))
        self.Config.scope_time.set(str(time))
        self.graph.draw()
        # tell microcontroller about the new setting
        DP = DataPackage()
        DP.addUInt8(Command_SetScopeSkips)
        DP.addUInt16(int(time/5))
        self.MCU.append_write_queue(DP)
        
        
    def send_pid_settings(self):
        if logging:
            print("SEND PID")
        # prepare data package to send to microcontroller and add it to the write queue
        DP = DataPackage()
        DP.addUInt8(Command_SetPIDParameters)
        DP.addFloat(float(self.Config.kp.get()))
        DP.addFloat(float(self.Config.ki.get()))
        DP.addFloat(float(self.Config.kd.get()))
        self.MCU.append_write_queue(DP)
        # write changes to config
        self.Config.Save()
        
        
    def auto_pid_settings(self):
        if logging:
            print("AUTO PID")
        
        
    def send_io_settings(self):
        if logging:
            print("SEND I/O")
        # adjust plot range
        self.ax1.set_ylim(PLOT_RANGES[self.Config.ADC_Setting.get()])
        self.graph.draw()
        # send command to microcontroller
        DP = DataPackage()
        DP.addUInt8(Command_SetIO)
        DP.addUInt8(ADC_CONFIG[self.Config.ADC_Setting.get()]) # ADC 1
        DP.addUInt8(ADC_CONFIG[self.Config.ADC_Setting.get()]) # ADC 2
        DP.addUInt8(DAC_CONFIG[self.Config.DAC_Setting.get()]) # DAC 1
        DP.addUInt8(DAC_CONFIG[self.Config.DAC_Setting.get()]) # DAC 2, unused
        self.MCU.append_write_queue(DP)
        # write changes to config
        self.Config.Save()
        
        
    def linearize(self):
        if logging:
            print("LINEARIZE")
        # open window that performs the calibration
        LinearizeWindow(self)
        
        
    def setup_window(self):
        # global window settings
        self.winfo_toplevel().title("PowerStar")
        self.winfo_toplevel().config(bg='white')
        self.geometry("1024x768")
        self.resizable(False, False)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        self.canvas = tk.Canvas(self, width=1024, height=768)
        self.canvas.pack()
        # draw background
        background_image = Image.open("concert.png")
        self.background_image = ImageTk.PhotoImage(background_image)
        self.canvas.create_image(0,0, image=self.background_image, anchor = tk.NW)
        # draw title
        self.canvas.create_rectangle(20,  20, 310, 60, fill="black", stipple="gray75", width=0)
        self.canvas.create_text(160, 40, text='POWERSTAR', font=('Broadway', 32), fill='white')
        # show lock name
        self.namewidget = tk.Label(self, textvar=self.Config.name, font=('Broadway', 18), foreground='white', background='black')
        self.namewidget.place(anchor=tk.NE, x=990, y=20)
        self.changenamebutton = tk.Button(self, text="Change", font=('Copperplate Gothic Bold', 10), foreground='white', background='black', command=self.changeName)
        self.changenamebutton.place(anchor=tk.NE, x=990, y=60)
        
        
    def setup_plots(self):
        scope_time = int(self.Config.scope_time.get())
        self.plotrange = np.linspace(0,scope_time,int(READ_FLOATS/2))
        # fig
        plt.style.use('dark_background')
        mpl.rcParams['font.family'] = 'serif'
        mpl.rcParams['font.serif'] = ['Copperplate Gothic Bold']
        mpl.rcParams['axes.unicode_minus']=False
        fig = plt.Figure(figsize=(9,3.5))
        # ax 1: inputs
        self.ax1 = fig.add_axes([0.2, 0.15, 0.32, 0.75])
        self.ax1.grid()
        self.ax1.set_xlim(0,scope_time)
        self.ax1.set_ylim(PLOT_RANGES[self.Config.ADC_Setting.get()])
        self.ax1.set_xlabel("Time (ms)")
        self.ax1.set_ylabel("Voltage (V)")
        self.ax1.set_title("Inputs")
        legend = [mpl.lines.Line2D([0], [0], color='C0', lw=4), mpl.lines.Line2D([0], [0], color='C1', lw=4)]
        self.ax1.legend(legend, ['Set Point', 'Actual Value'], loc='upper center', prop={'size': 8}, ncol=2)
        # ax 2: error signal
        self.ax2 = fig.add_axes([0.65, 0.15, 0.32, 0.75])
        self.ax2.grid()
        self.ax2.set_xlim(0,scope_time)
        self.ax2.set_xlabel("Time (ms)")
        self.ax2.set_ylabel("Voltage (V)")
        self.ax2.set_title("Error")
        ticks = np.arange(SCOPE_LOG_CUTOFF-1, -SCOPE_LOG_CUTOFF+2)
        self.ax2.set_yticks(ticks)
        self.ax2.set_yticklabels([np.sign(tick)*(10.0**(SCOPE_LOG_CUTOFF+abs(tick))) for tick in ticks])
        self.ax2.set_ylim(SCOPE_LOG_CUTOFF-1.2, -SCOPE_LOG_CUTOFF+1.2)
        self.graph = FigureCanvasTkAgg(fig, master=self)
        self.graph.get_tk_widget().place(x=50, y=100)
        # buttons to change the time scale
        tk.Button(self, text="1\ns", command=(lambda: self.changeScopeTime(1000)), font=('Copperplate Gothic Bold', self.fontsize), width=6, foreground='white', background='black').place(x=60, y=130)
        tk.Button(self, text="250\nms", command=(lambda: self.changeScopeTime(250)),font=('Copperplate Gothic Bold', self.fontsize), width=6, foreground='white', background='black').place(x=60, y=190)
        tk.Button(self, text="70\nms", command=(lambda: self.changeScopeTime(70)),font=('Copperplate Gothic Bold', self.fontsize), width=6, foreground='white', background='black').place(x=60, y=250)
        tk.Button(self, text="20\nms", command=(lambda: self.changeScopeTime(20)),font=('Copperplate Gothic Bold', self.fontsize), width=6, foreground='white', background='black').place(x=60, y=310)
        tk.Button(self, text="5\nms", command=(lambda: self.changeScopeTime(5)),font=('Copperplate Gothic Bold', self.fontsize), width=6, foreground='white', background='black').place(x=60, y=370)
        # fake data
        xr = np.linspace(0,1000,20000)
        noise = np.random.normal(0, 0.01, 20000)
        logify(noise, cutoff=SCOPE_LOG_CUTOFF)
        self.ax2.plot(xr, noise, c='C2')
        input1 = [2.5+2*np.sin(6.3*1*t) for t in xr]
        input2= [2.5+2*np.sin(6.3*1*t-0.3) for t in xr]
        self.ax1.plot(xr, input1, c='C0')
        self.ax1.plot(xr, input2, c='C1')
        
        
    def setup_pid_box(self):
        # pid frame
        pidframe = tk.Frame(self, background='black', padx=3, pady=3)
        pidframe.place(anchor=tk.NW, x=50, y=470)
        tk.Label(pidframe, text='PID ', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=2)
        # k_p
        tk.Label(pidframe, text="kp", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=1.0, increment=0.0001, borderwidth=0, textvar=self.Config.kp, width=6, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=1, column=1)
        # k_i
        tk.Label(pidframe, text="ki", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=1.0, increment=0.0001, borderwidth=0, textvar=self.Config.ki, width=6, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=2, column=1)
        # k_d
        tk.Label(pidframe, text="kd", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=1.0, increment=0.0001, borderwidth=0, textvar=self.Config.kd, width=6, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=3, column=1)
        # buttons
        tk.Button(pidframe, text="Auto...", command=self.auto_pid_settings, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=4, column=0)
        tk.Button(pidframe, text="Send", command=self.send_pid_settings, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=4, column=1)
        
        
    def setup_io_box(self):
        # input / output settings
        ioframe = tk.Frame(self, background='black', padx=3, pady=3)
        tk.Label(ioframe, text='In- and Output ', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=2)
        ioframe.place(anchor=tk.NW, x=300, y=470)
        # input
        tk.Label(ioframe, text="Input", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        om_inputsettings = tk.OptionMenu(ioframe, self.Config.ADC_Setting, "0-5V", "0-10V", "+/- 5V", "+/- 10V")
        om_inputsettings.config(borderwidth=0, highlightthickness=0, indicatoron=0, relief=tk.FLAT, width=6, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black')
        om_inputsettings.grid(row=1, column=1)
        # output
        tk.Label(ioframe, text="Output*", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        om_outputsettings = tk.OptionMenu(ioframe, self.Config.DAC_Setting, "0-5V", "0-10V", "+/- 5V", "+/- 10V")
        om_outputsettings.config(borderwidth=0, highlightthickness=0, indicatoron=0, relief=tk.FLAT, width=6, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black')
        om_outputsettings.grid(row=2, column=1)
        # note
        tk.Label(ioframe, text="* Output range is defined\nby jumpers on the PCB!", font=('Copperplate Gothic Bold', 8), foreground='white', background='black').grid(row=3, column=0, columnspan=2)
        # button
        tk.Button(ioframe, text="Send", command=self.send_io_settings, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=4, column=0, columnspan=2)
        
        
    def setup_lin_box(self):
        # linearization
        linframe = tk.Frame(self, background='black', padx=3, pady=3)
        linframe.place(anchor=tk.NW, x=600, y=470)
        tk.Label(linframe, text='AOM Linearization', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0, column=0, columnspan=2)
        # timestamp
        self.lin_timestamp = tk.Label(linframe, text="", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.W)
        self.lin_timestamp.grid(row=1, column=0, columnspan=2)
        # make figure
        fig_lin = plt.Figure(figsize=(3.5,2))
        self.ax_lin = fig_lin.add_axes([0.2, 0.3, 0.7, 0.6])
        self.ax_lin.set_xlabel("AOM Input (V)")
        self.ax_lin.set_ylabel("Photo Diode (V)")
        self.graph_lin = FigureCanvasTkAgg(fig_lin, master=linframe)
        self.graph_lin.get_tk_widget().grid(row=2, column=0, columnspan=2)
        # buttons
        tk.Button(linframe, text="Delete Calibration", command=self.delete_lin, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=3, column=0)
        tk.Button(linframe, text="Calibrate Now", command=self.linearize, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=3, column=1)
        self.redraw_lin()
            
    def delete_lin(self):
        self.Config.lin_timestamp.set('None')
        self.Config.lin_pivots.set('None')
        self.redraw_lin()
        
    def redraw_lin(self):
        print(f'linpivots: {self.Config.lin_pivots}')
        if self.Config.lin_pivots.get() == 'None':
            self.ax_lin.set_xlim(0,1)
            self.ax_lin.set_ylim(0,1)
            # remove everything
            while len(self.ax_lin.lines)>0:
                self.ax_lin.lines.pop(0)
            while len(self.ax_lin.collections)>0:
                self.ax_lin.collections.pop(0)
            while len(self.ax_lin.texts)>0:
                self.ax_lin.texts.pop(0)
            self.ax_lin.text(0.5, 0.5, "No calibration yet", ha='center', va='center', c='C3')
            self.ax_lin.grid(False)
        else:
            x = self.Config.GetLinPivots()[:,0]
            y = self.Config.GetLinPivots()[:,1]
            # remove everything
            while len(self.ax_lin.lines)>0:
                self.ax_lin.lines.pop(0)
            while len(self.ax_lin.collections)>0:
                self.ax_lin.collections.pop(0)
            while len(self.ax_lin.texts)>0:
                self.ax_lin.texts.pop(0)
            self.ax_lin.scatter(x, y, c='C3', s=10)
            self.ax_lin.set_xlim(x[0]-(x[-1]-x[0])/20, x[-1]+(x[-1]-x[0])/20)
            self.ax_lin.set_ylim(y[0]-(y[-1]-y[0])/20, y[-1]+(y[-1]-y[0])/20)
            self.ax_lin.grid()
        self.graph_lin.draw()
        self.lin_timestamp.config(text=('Last Calibration: '+self.Config.lin_timestamp.get()))


    def check_read_queue(self):
        if self.MCU.mode=='run':
            if len(self.MCU.read_queue)>0:
                # clear figure
                while len(self.ax1.lines)>0:
                    self.ax1.lines.pop(0)
                while len(self.ax2.lines)>0:
                    self.ax2.lines.pop(0)
                # get new data from read queue
                data = self.MCU.pop_read_queue()
                data_pd = [data[2*i] for i in range(int(READ_FLOATS/2))]
                data_set = [data[2*i+1] for i in range(int(READ_FLOATS/2))]
                data_error = [data[2*i+4]-data[2*i+1] for i in range(int(READ_FLOATS/2)-2)]
                logify(data_error)
                # plot
                self.ax1.plot(self.plotrange, data_pd, c='C0')
                self.ax1.plot(self.plotrange, data_set, c='C1')
                self.ax2.plot(self.plotrange[:-2], data_error, c='C2')
                self.graph.draw()
        self.after(100, self.check_read_queue)
                
        
        

window = ControlGUI()

# Install the Reactor support
#tksupport.install(window)

window.after(100, window.check_read_queue)
window.mainloop()
