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
import time

from scope import *
from macros import *
from plotscan import PlotScan
from spi import *
from settings import Settings

import analyzescan

logging = True

class ControlGUI(tk.Tk):
    def __init__(self):
        tk.Tk.__init__(self)
        
        self.fontsize = 12
        
        # set up and load configuration (PID parameters, I/O settings, window properties...)
        self.Config = Settings(self)
        
        # makae GUI
        self.setup_window()
        self.setup_plots()
        self.setup_pid_box()
        self.setup_io_box()
        self.setup_live_box()
        
        # set up SPI to microcontroller and read/write queue
        self.MCU = MCU_Handler(self)
        # make sure all MCU threads stop when window is closed
        self.protocol("WM_DELETE_WINDOW", self.close)
        
    def close(self):
        self.MCU.stop_event.set()
        self.destroy()

    def changeName(self):
        self.Config.name.set(tk.simpledialog.askstring("Change Name", "Enter new name:", initialvalue=self.namewidget.cget("text")))
        #self.namewidget.config(text = self.Config.name.get())
        
        
    def scan_now(self):
        # tell microcontroller to perform scan
        DP = DataPackage()
        DP.addUInt8(Command_MeasureAOMResponse)
        DP.addUInt8(1) # ADC Channel
        DP.addUInt8(2) # DAC Channel
        self.root.MCU.append_write_queue(DP)
        # stop MCU polling
        self.root.MCU.mode = 'stop'
        
        # wait until the write queue is empty and all ongoing communication has stopped
        while len(self.root.MCU.write_queue)>0:
            pass
        time.sleep(2)
        # if something is in the read queue, drop it
        while len(self.root.MCU.read_queue)>0:
            self.root.MCU.pop_read_queue()
        # with the next rising flank on the GPIO, calibration data is ready
        while not self.root.MCU.get_GPIO_pin():
            pass
        # read data from MCU
        self.root.MCU.transfer(sendbytes=0, readfloats=1000)
        data = self.root.MCU.pop_read_queue()
        signal1 = [data[2*i] for i in range(500)]
        signal2 = [data[2*i+1] for i in range(500)]
        self.scan_plot.plot()
        
        
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
        
        
    def scan(self):
        if logging:
            print("SCAN")
            
        # set up trace variables
        self.trace_from = float(self.Config.trace_from.get())
        self.trace_to = float(self.Config.trace_to.get())
        self.trace_stepsize = float(self.Config.trace_stepsize.get())
        self.trace_length = int(np.ceil((self.trace_to - self.trace_from) / self.trace_stepsize)+1) # +1 to have both start and end point included
        self.trace_voltages = np.linspace(self.trace_from, self.trace_to, self.trace_length)
        
        # send command to MCU
        DP = DataPackage()
        DP.addUInt8(Command_RecordTrace)
        DP.addUInt8(1) # record channel 1: true
        DP.addUInt8(1) # record channel 2: true
        DP.addUInt8(2) # output channel 2
        DP.addFloat(self.trace_from) # from
        DP.addFloat(self.trace_to) # to
        DP.addUInt32(self.trace_length) # trace length
        self.MCU.append_write_queue(DP)
        
        # wait until the write queue is empty and all ongoing communication has stopped
        self.MCU.mode = 'stop'
        while len(self.MCU.write_queue)>0:
            pass
        time.sleep(2)
        # if something is in the read queue, drop it
        while len(self.MCU.read_queue)>0:
            self.MCU.pop_read_queue()
        # with the next rising flank on the GPIO, calibration data is ready
        while not self.MCU.get_GPIO_pin():
            pass
        # read data from MCU
        self.MCU.transfer(sendbytes=0, readfloats=2*int(self.trace_length))
        #print(self.root.MCU.read_queue)
        self.trace_data = self.MCU.pop_read_queue()
        self.trace_transmission = [self.trace_data[2*i] for i in range(self.trace_length)]
        self.trace_errorsignal = [self.trace_data[2*i+1] for i in range(self.trace_length)]
        print(self.trace_transmission[:10])
        print(self.trace_errorsignal[:10])
        
        # plot new data
        self.scan_plot.plot(self.trace_voltages, self.trace_transmission, self.trace_errorsignal, {'c':'C0'}, {'c':'C1'})
        
        # save data
        np.savetxt("last_scan.txt", [self.trace_voltages, self.trace_transmission, self.trace_errorsignal])
        
        # restart live stream
        self.MCU.mode = 'run'
        
    def setup_window(self):
        # global window settings
        self.winfo_toplevel().title("FrequencyStar")
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
        self.canvas.create_text(20, 20, text='FREQUENCYSTAR', font=('Broadway', 32), fill='white', anchor=tk.NW)
        # show lock name
        self.namewidget = tk.Label(self, textvar=self.Config.name, font=('Broadway', 18), foreground='white', background='black')
        self.namewidget.place(anchor=tk.NE, x=990, y=20)
        self.changenamebutton = tk.Button(self, text="Change", font=('Copperplate Gothic Bold', 10), foreground='white', background='black', command=self.changeName)
        self.changenamebutton.place(anchor=tk.NE, x=990, y=60)
        
        
    def setup_plots(self):
        plotframe = tk.Frame(self, background='black', padx=3, pady=3)
        plotframe.place(anchor=tk.NW, x=50, y=100)
        # control panel
        tk.Label(plotframe, text='Scan                   ', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=3)
        
        tk.Label(plotframe, text="From:", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        tk.Spinbox(plotframe, from_=-10.0, to=10.0, increment=1.0, borderwidth=0, textvar=self.Config.trace_from, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=1, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=2)
        
        tk.Label(plotframe, text="To:", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        tk.Spinbox(plotframe, from_=-10.0, to=10.0, increment=1.0, borderwidth=0, textvar=self.Config.trace_to, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=2, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=2)
        
        tk.Label(plotframe, text="Step Size:", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=0)
        tk.Spinbox(plotframe, from_=0.0, to=1.0, increment=0.001, borderwidth=0, textvar=self.Config.trace_stepsize, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=3, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=2)
        
        tk.Button(plotframe, text="Scan Now", command=self.scan, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=4, column=0, columnspan=3)
        
        tk.Label(plotframe, text="Flank:", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=6, column=0)
        OM_flank = tk.OptionMenu(plotframe, self.Config.lock_flank, "FALLING", "RISING")
        OM_flank.config(borderwidth=0, highlightthickness=0, indicatoron=0, relief=tk.FLAT, width=7, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black')
        OM_flank.grid(row=6, column=1)
        
        tk.Label(plotframe, text="Det. Thresh.:", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=7, column=0)
        tk.Spinbox(plotframe, from_=0, to=1.0, increment=0.01, borderwidth=0, textvar=self.Config.lock_relsens, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=7, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=7, column=2)
        
        tk.Label(plotframe, text="Offset:", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=8, column=0)
        tk.Spinbox(plotframe, from_=-10.0, to=10.0, increment=0.001, borderwidth=0, textvar=self.Config.lock_offset, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', buttonbackground='black').grid(row=8, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black', anchor=tk.E).grid(row=8, column=2)
        
        self.Config.lock_offset.trace_add('write', lambda name, index, mode: self.calculate_lockpoints())
        self.Config.lock_relsens.trace_add('write', lambda name, index, mode: self.calculate_lockpoints())
        self.Config.lock_flank.trace_add('write', lambda name, index, mode: self.calculate_lockpoints())
        
        # fig
        plt.style.use('dark_background')
        mpl.rcParams['font.family'] = 'serif'
        mpl.rcParams['font.serif'] = ['Copperplate Gothic Bold']
        mpl.rcParams['font.size'] = 10
        mpl.rcParams['axes.unicode_minus']=False
        self.scan_plot = PlotScan(plotframe, 7.25, 3.5)
        self.scan_plot.grid(row=0, column=3, rowspan=20)
        # get last data
        self.piezo, self.transm, self.errors = self.Config.GetLockTrace()
        self.scan_plot.plot(self.piezo, self.transm, self.errors, kwargs1={'c':'C0'}, kwargs2={'c':'C1'})
        self.calculate_lockpoints()
        
    def calculate_lockpoints(self):
        print("calculate lockpoint")
        time.sleep(0.1)
        flanks = analyzescan.get_flanks(self.piezo, self.errors, float(self.Config.lock_relsens.get()), flank=FLANK[self.Config.lock_flank.get()])
        print("flanks done")
        time.sleep(0.1)
        lockpoints = analyzescan.get_lock_points_from_flanks(self.piezo, self.errors, flanks, float(self.Config.lock_offset.get()))
        print("lockpoints done")
        time.sleep(0.1)
        self.scan_plot.mark(lockpoints)
        self.scan_plot.plot_lock_points(self.piezo, flanks, lockpoints)
        
    def change_offset(self):
        offset = float(self.Config.lock_offset.get())
        self.scan_plot.hline(offset)
        self.scan_plot.mark(analyzescan.get_lock_points(self.piezo, self.transm, self.errors, offset), offset)
        
        
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
        
        
    def setup_live_box(self):
        # linearization
        liveframe = tk.Frame(self, background='black', padx=3, pady=3)
        liveframe.place(anchor=tk.NW, x=600, y=470)
        tk.Label(liveframe, text='Live Data', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0, column=0, columnspan=2)
        # make figure
        fig_live = plt.Figure(figsize=(3.5,2))
        self.ax_live = fig_live.add_axes([0.2, 0.25, 0.6, 0.7])
        self.ax_live.set_xlabel("Time (s)")
        self.ax_live.set_ylabel("Transm. (V)", c='C2')
        self.ax_live2 = self.ax_live.twinx()
        self.ax_live2.set_ylabel("Error (V)", c='C1')
        self.ax_live.tick_params(axis='y', colors='C2')
        self.ax_live2.tick_params(axis='y', colors='C1')
        self.graph_live = FigureCanvasTkAgg(fig_live, master=liveframe)
        self.graph_live.get_tk_widget().grid(row=2, column=0, columnspan=2)
        # buttons
        #tk.Button(linframe, text="Delete Calibration", command=self.delete_lin, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=3, column=0)
        #tk.Button(linframe, text="Calibrate Now", command=self.linearize, font=('Copperplate Gothic Bold', self.fontsize), foreground='white', background='black').grid(row=3, column=1)
        self.redraw_live()
            
    def delete_live(self):
        self.redraw_live()
        
    def redraw_live(self):
        self.graph_live.draw()


    def check_read_queue(self):
        if self.MCU.mode=='run':
            if len(self.MCU.read_queue)>0:
                # clear figure
                while len(self.ax_live.lines)>0:
                    self.ax_live.lines.pop(0)
                # get new data from read queue
                data = self.MCU.pop_read_queue()
                data_transmission = [data[2*i] for i in range(int(READ_FLOATS/2))]
                data_errorsignal = [data[2*i+1] for i in range(int(READ_FLOATS/2))]
                print(data_transmission[:10])
                print(data_errorsignal[:10])
                # plot
                self.ax_live.plot(data_transmission, c='C0')
                self.ax_live.plot(data_errorsignal, c='C1')
                self.graph_live.draw()
        self.after(100, self.check_read_queue)
                
        
        

window = ControlGUI()

# Install the Reactor support
#tksupport.install(window)

window.after(100, window.check_read_queue)
window.mainloop()