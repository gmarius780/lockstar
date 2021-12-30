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
from linearize import LinearizeWindow
import datetime
import time

from scope import *
from macros import *
from plotscan import PlotScan
from spi import *
from settings import Settings
from autopid import AutoPIDWindow

import analyzescan

logging = True
fontsize = 12

class ControlGUI(tk.Tk):
    def __init__(self, name):
        tk.Tk.__init__(self)
        
        # set up and load configuration (PID parameters, I/O settings, window properties...)
        self.Config = Settings(self)
        
        # set up SPI to microcontroller and read/write queue
        self.MCU = MCU_Handler(self, speed=20000000)
        # make sure all MCU threads stop when window is closed
        self.protocol("WM_DELETE_WINDOW", self.close)
        
        # setup window
        # global window settings
        self.winfo_toplevel().title(name)
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
        self.canvas.create_text(20, 20, text=name, font=('Broadway', 32), fill='white', anchor=tk.NW)
        # show lock name
        self.namewidget = tk.Label(self, textvar=self.Config.name, font=('Broadway', 18), foreground='white', background='black')
        self.namewidget.place(anchor=tk.NE, x=990, y=20)
        self.changenamebutton = tk.Button(self, text="Change", font=('Copperplate Gothic Bold', 10), foreground='white', background='black', command=self.changeName)
        self.changenamebutton.place(anchor=tk.NE, x=990, y=60)
        
        # setup mpl
        plt.style.use('dark_background')
        mpl.rcParams['font.family'] = 'serif'
        mpl.rcParams['font.serif'] = ['Copperplate Gothic Bold']
        mpl.rcParams['font.size'] = 10
        mpl.rcParams['axes.unicode_minus']=False
        
    def close(self):
        self.MCU.stop_event.set()
        self.destroy()

    def changeName(self):
        self.Config.name.set(tk.simpledialog.askstring("Change Name", "Enter new name:", initialvalue=self.namewidget.cget("text")))
        #self.namewidget.config(text = self.Config.name.get())
        
    def add_pid_box(self, x, y, pid_id=1, name='PID '):
        pid_box = PIDBox(self, x, y, pid_id, name)
        
    def add_sv_box(self, x, y):
        sv_box = SVBox(self, x, y)
        
    def add_live_box(self, x, y, label1="", label2="", wx=300, wy=200):
        live_box = LiveBox(self, x, y, label1, label2, wx, wy)
        
    def add_io_box(self, x, y):
        io_box = IOBox(self, x, y)
        
    def add_scan_box(self, x, y, lockpoints_function, waypoints_function):
        scan_box = ScanBox(self, x, y, lockpoints_function, waypoints_function)
        
    def add_cal_box(self, x, y):
        cal_box = CalBox(self, x, y)
        
    def add_fftcorrection_box(self, x, y, pid_id=1, name='FFTCorrection '):
        fftcorrection_box = FFTCorrectionBox(self, x, y, pid_id, name)
        
        
        
        
class IOBox:
    def __init__(self, parent, x, y):
        self.parent = parent
        # input / output settings
        ioframe = tk.Frame(parent, background='black', padx=3, pady=3)
        tk.Label(ioframe, text='In- and Output ', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=2)
        ioframe.place(anchor=tk.NW, x=x, y=y)
        # input
        tk.Label(ioframe, text="Input", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        om_inputsettings = tk.OptionMenu(ioframe, self.parent.Config.ADC_Setting, "0-5V", "0-10V", "+/- 5V", "+/- 10V")
        om_inputsettings.config(borderwidth=0, highlightthickness=0, indicatoron=0, relief=tk.FLAT, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black')
        om_inputsettings.grid(row=1, column=1)
        # output
        tk.Label(ioframe, text="Output*", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        om_outputsettings = tk.OptionMenu(ioframe, self.parent.Config.DAC_Setting, "0-5V", "0-10V", "+/- 5V", "+/- 10V")
        om_outputsettings.config(borderwidth=0, highlightthickness=0, indicatoron=0, relief=tk.FLAT, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black')
        om_outputsettings.grid(row=2, column=1)
        # note
        tk.Label(ioframe, text="* Output range is defined\nby jumpers on the PCB!", font=('Copperplate Gothic Bold', 8), foreground='white', background='black').grid(row=3, column=0, columnspan=2)
        # button
        tk.Button(ioframe, text="Send", command=self.send_io_settings, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=4, column=0, columnspan=2)
        
    def send_io_settings(self):
        if logging:
            print("SEND I/O")
        # send command to microcontroller
        DP = DataPackage()
        DP.addUInt8(Command_SetIO)
        DP.addUInt8(ADC_CONFIG[self.parent.Config.ADC_Setting.get()]) # ADC 1
        DP.addUInt8(ADC_CONFIG[self.parent.Config.ADC_Setting.get()]) # ADC 2
        DP.addUInt8(DAC_CONFIG[self.parent.Config.DAC_Setting.get()]) # DAC 1
        DP.addUInt8(DAC_CONFIG[self.parent.Config.DAC_Setting.get()]) # DAC 2, unused
        self.parent.MCU.append_write_queue(DP)
        # write changes to config
        self.parent.Config.Save()        
     


class SVBox:
    def __init__(self, parent, x, y):
        self.parent = parent
        # title
        svframe = tk.Frame(parent, background='black', padx=3, pady=3)
        tk.Label(svframe, text='Set Values ', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=2)
        svframe.place(anchor=tk.NW, x=x, y=y)
        # sv 1
        tk.Label(svframe, text="Channel 1", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        self.sv1 = tk.StringVar(parent)
        self.sv1.set(0.0)
        tk.Spinbox(svframe, from_=-10.0, to=10.0, increment=0.01, borderwidth=0, textvar=self.sv1, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=1, column=1)
        # sv 2
        tk.Label(svframe, text="Channel 2", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        self.sv2 = tk.StringVar(parent)
        self.sv2.set(0.0)
        tk.Spinbox(svframe, from_=-10.0, to=10.0, increment=0.01, borderwidth=0, textvar=self.sv2, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=2, column=1)
        # button
        tk.Button(svframe, text="Send", command=self.send_sv_settings, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=3, column=0, columnspan=2)
        
    def send_sv_settings(self):
        if logging:
            print("SEND Setvalue")
        # send command to microcontroller
        DP = DataPackage()
        DP.addUInt8(Command_ProgramSetValue)
        DP.addFloat(float(self.sv1.get())) 
        print(float(self.sv1.get()))
        DP.addFloat(float(self.sv2.get())) 
        print(float(self.sv2.get()))
        self.parent.MCU.append_write_queue(DP)

           

class PIDBox:
    def __init__(self, parent, x, y, pid_id=1, name='PID '):
        self.pid_id = pid_id
        self.parent = parent
        # pid frame
        pidframe = tk.Frame(parent, background='black', padx=3, pady=3)
        pidframe.place(anchor=tk.NW, x=x, y=y)
        tk.Label(pidframe, text=name, anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=2)
        # k_p
        tk.Label(pidframe, text="kp", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=1.0, increment=0.0001, borderwidth=0, textvar=self.parent.Config.kp, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=1, column=1)
        # k_i
        tk.Label(pidframe, text="ki", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=1.0, increment=0.0001, borderwidth=0, textvar=self.parent.Config.ki, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=2, column=1)
        # k_d
        tk.Label(pidframe, text="kd", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=1.0, increment=0.0001, borderwidth=0, textvar=self.parent.Config.kd, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=3, column=1)
        # buttons
        tk.Button(pidframe, text="Tune...", command=self.auto_pid_settings, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=4, column=0)
        tk.Button(pidframe, text="Send", command=self.send_pid_settings, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=4, column=1)
                
    def send_pid_settings(self):
        if logging:
            print("SEND PID")
        # prepare data package to send to microcontroller and add it to the write queue
        DP = DataPackage()
        DP.addUInt8(Command_SetPIDParameters)
        DP.addUInt8(self.pid_id)
        DP.addFloat(float(self.parent.Config.kp.get()))
        DP.addFloat(float(self.parent.Config.ki.get()))
        DP.addFloat(float(self.parent.Config.kd.get()))
        self.parent.MCU.append_write_queue(DP)
        # write changes to config
        self.parent.Config.Save()
        
    def auto_pid_settings(self):
        if logging:
            print("AUTO PID")
        AutoPIDWindow(self)
         
            
            
            
class LiveBox:
    def __init__(self, parent, x, y, label1, label2, wx, wy):
        self.parent = parent
        liveframe = tk.Frame(parent, background='black', padx=3, pady=3)
        liveframe.place(anchor=tk.NW, x=x, y=y)
        tk.Label(liveframe, text='Live Data', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0, column=0, columnspan=2)
        # make figure
        fig_live = plt.Figure(figsize=(3.5,2))
        self.ax_live = fig_live.add_axes([0.2, 0.25, 0.6, 0.7])
        self.ax_live.set_xlabel("Time (s)")
        self.ax_live.set_ylabel(label1, c='C2')
        self.ax_live2 = self.ax_live.twinx()
        self.ax_live2.set_ylabel(label2, c='C1')
        self.ax_live.tick_params(axis='y', colors='C2')
        self.ax_live2.tick_params(axis='y', colors='C1')
        self.graph_live = FigureCanvasTkAgg(fig_live, master=liveframe)
        self.graph_live.get_tk_widget().grid(row=2, column=0, columnspan=2)
        parent.after(100, self.check_read_queue)
        self.ax_live.set_ylim(-11,11)
        self.ax_live2.set_ylim(-11,11)
        
    def check_read_queue(self):
        if self.parent.MCU.mode=='run':
            if len(self.parent.MCU.read_queue)>0:
                # clear figure
                while len(self.ax_live.lines)>0:
                    self.ax_live.lines.pop(0)
                while len(self.ax_live2.lines)>0:
                    self.ax_live2.lines.pop(0)
                # get new data from read queue
                data = self.parent.MCU.pop_read_queue()
                #print(data)
                data_transmission = [data[2*i] for i in range(int(READ_FLOATS/2))]
                data_errorsignal = [data[2*i+1] for i in range(int(READ_FLOATS/2))]
                #print(data_transmission[:10])
                #print(data_errorsignal[:10])
                # plot
                self.ax_live.plot(data_transmission, c='C0')
                self.ax_live2.plot(data_errorsignal, c='C1')
                self.graph_live.draw()
        self.parent.after(100, self.check_read_queue)



class ScanBox:
    def __init__(self, parent, x, y, lockpoints_function, waypoints_function):
        self.lockpoints_function = lockpoints_function
        self.waypoints_function = waypoints_function
        self.parent = parent
        plotframe = tk.Frame(parent, background='black', padx=3, pady=3)
        plotframe.place(anchor=tk.NW, x=50, y=100)
        # control panel
        tk.Label(plotframe, text='Scan                   ', anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=3)
        
        tk.Label(plotframe, text="From:", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        tk.Spinbox(plotframe, from_=-10.0, to=10.0, increment=1.0, borderwidth=0, textvar=self.parent.Config.trace_from, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=1, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=2)
        
        tk.Label(plotframe, text="To:", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        tk.Spinbox(plotframe, from_=-10.0, to=10.0, increment=1.0, borderwidth=0, textvar=self.parent.Config.trace_to, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=2, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=2)
        
        tk.Label(plotframe, text="Step Size:", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=0)
        tk.Spinbox(plotframe, from_=0.0, to=1.0, increment=0.001, borderwidth=0, textvar=self.parent.Config.trace_stepsize, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=3, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=2)
        
        tk.Button(plotframe, text="Scan Now", command=self.detailed_trace, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=4, column=0, columnspan=3)
        
        tk.Label(plotframe, text="Flank:", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=6, column=0)
        OM_flank = tk.OptionMenu(plotframe, self.parent.Config.lock_flank, "FALLING", "RISING")
        OM_flank.config(borderwidth=0, highlightthickness=0, indicatoron=0, relief=tk.FLAT, width=7, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black')
        OM_flank.grid(row=6, column=1)
        
        tk.Label(plotframe, text="Det. Thresh.:", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=7, column=0)
        tk.Spinbox(plotframe, from_=0, to=1.0, increment=0.01, borderwidth=0, textvar=self.parent.Config.lock_relsens, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=7, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=7, column=2)
        
        tk.Label(plotframe, text="Offset:", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=8, column=0)
        tk.Spinbox(plotframe, from_=-10.0, to=10.0, increment=0.001, borderwidth=0, textvar=self.parent.Config.lock_offset, justify=tk.RIGHT, width=5, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=8, column=1)
        tk.Label(plotframe, text=" V", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=8, column=2)
        
        tk.Button(plotframe, text="Find Lines", command=self.calculate_lockpoints, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=9, column=0, columnspan=3)
        
        tk.Button(plotframe, text="Lock", command=self.go_to_lock, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=10, column=0, columnspan=1)
        tk.Button(plotframe, text="Unlock", command=self.unlock, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=10, column=1)
        
        # fig
        self.scan_plot = PlotScan(plotframe, 7.25, 3.5)
        self.scan_plot.grid(row=0, column=3, rowspan=20)
        
    def calculate_lockpoints(self):
        print("calculate lockpoint")
        #self.flanks = analyzescan.get_flanks(self.voltage, self.errors, float(self.parent.Config.lock_relsens.get()), flank=FLANK[self.parent.Config.lock_flank.get()])
        #print("flanks done")
        #self.lockpoints = analyzescan.get_lock_points_from_flanks(self.voltage, self.errors, self.flanks, float(self.parent.Config.lock_offset.get()))
        self.lockpoints, self.flanks, self.extrainformation = self.lockpoints_function(self.voltage, self.transm, self.errors, flank=FLANK[self.parent.Config.lock_flank.get()], rel_threshold=float(self.parent.Config.lock_relsens.get()), offset_shift=float(self.parent.Config.lock_offset.get()))
        print("lockpoints done")
        
        lockpoints_voltage = np.array([[self.voltage[lockpoint[0]], lockpoint[1]] for lockpoint in self.lockpoints])
        self.scan_plot.mark(lockpoints_voltage)
        print("A")
        self.scan_plot.plot_lock_points(self.voltage, self.flanks, self.lockpoints)
        print("B")
        
    def unlock(self):
        print("unlock")
        # send command to MCU
        DP = DataPackage()
        DP.addUInt8(Command_StopLock)
        self.parent.MCU.append_write_queue(DP)
        
        
        
    def scan(self):
        if logging:
            print("SCAN")
            
        # set up trace variables
        self.trace_from = float(self.Config.trace_from.get())
        self.trace_to = float(self.Config.trace_to.get())
        self.trace_stepsize = float(self.Config.trace_stepsize.get())
        self.trace_length = int(np.ceil((self.trace_to - self.trace_from) / self.trace_stepsize)+1) # +1 to have both start and end point included
        if self.trace_length>4000:
                self.trace_length=4000;
        self.piezo = np.linspace(self.trace_from, self.trace_to, self.trace_length)
        
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
        self.transm = np.array([self.trace_data[2*i] for i in range(self.trace_length)])
        self.errors = np.array([self.trace_data[2*i+1] for i in range(self.trace_length)])
        self.Config.SetLockTrace(self.piezo, self.transm, self.errors)
        
        # plot new data
        self.scan_plot.plot(self.piezo, self.transm, self.errors, {'c':'C0'}, {'c':'C1'})
        
        self.ax_live.set_ylim(1.1*np.min(self.transm)-0.1*np.max(self.transm),1.1*np.max(self.transm)-0.1*np.min(self.transm))
        self.ax_live2.set_ylim(1.1*np.min(self.errors)-0.1*np.max(self.errors),1.1*np.max(self.errors)-0.1*np.min(self.errors))
        
        # save data
        np.savetxt("last_scan.txt", [self.piezo, self.transm, self.errors])
        
        # restart live stream
        self.MCU.mode = 'run'
        
        
        
        
        

        
    def testrc(self):
        DP = DataPackage()
        DP.addUInt8(Command_DetailedTrace)
        DP.addUInt8(1)
        DP.addUInt8(1)
        DP.addUInt8(2) # output channel 2
        DP.addUInt32(524288)
        DP.addUInt32(1000000)
        DP.addUInt32(500)
        self.MCU.append_write_queue(DP)
        time.sleep(10)
        
    def detailed_trace(self):
        DP = DataPackage()
        DP.addUInt8(Command_DetailedTrace)
        DP.addUInt8(1)
        DP.addUInt8(1)
        DP.addUInt8(2) # output channel 2
        DP.addUInt32(524288)
        DP.addUInt32(1000000)
        DP.addUInt32(500)
        self.parent.MCU.append_write_queue(DP)
        self.parent.MCU.mode = 'stop'
        while len(self.parent.MCU.write_queue)>0:
            pass
        time.sleep(2)
        output = []
        for i in range(2000):
            #print(i)
            while not self.parent.MCU.get_GPIO_pin():
                pass
            spiinput = self.parent.MCU.spi.readbytes(4000)
            values = unpack('1000f', bytes(spiinput))
            output += list(values)
            while self.parent.MCU.get_GPIO_pin():
                pass
        print("got it")
        print(output[:20])
        transm = [output[2*i] for i in range(1000000)]
        errors = [output[2*i+1] for i in range(1000000)]
        print(transm[:20])
        print(errors[:20])
        self.scan_plot.plot(np.linspace(-10,10,1000000), transm, errors, {'c':'C0'}, {'c':'C1'})
        self.voltage = np.linspace(-10,10,1000000)
        self.errors = np.array(errors)
        self.transm = np.array(transm)
        #np.savetxt("detailedtrace.txt", [errors,transm])
        self.parent.MCU.mode = 'run'
        
        
    def go_to_lock(self):
        print("go to")
        #print(float(self.scan_plot.marks[self.scan_plot.marked,0]))
        
        # calculate waypoints
        marked = self.scan_plot.marked
        #tolerance = (np.max(self.errors)-np.min(self.errors)) / 20
        #waypoints = analyzescan.generate_waypoints(self.piezo, self.errors, self.piezo[-1], self.lockpoints[marked,0], FLANK[self.Config.lock_flank.get()], tolerance, self.flanks[marked,1]-self.flanks[marked,0])
        #waypoints = [[1, 0.9*np.max(self.transm)], [-2, -0.025]]
        waypoints = self.waypoints_function(self.voltage, self.transm, self.errors, self.lockpoints[marked], self.extrainformation)
        #waypoints = deque([[3, 10], [5, 0], [2, 0.7434438880595923], [-2, 0.44590482418804167], [2, 0.7575066026363135], [-1, 0.015468984842300415]])
        no_waypoints = len(waypoints)
        
        print(waypoints)
        
        # send command to MCU
        DP = DataPackage()
        DP.addUInt8(Command_GotoLock)
        DP.addUInt8(2) # output channel 2
        DP.addUInt8(len(waypoints)) # how many waypoints
        DP.addFloat(0.00002) # move in these steps
        DP.addUInt8(1 if self.parent.Config.lock_flank.get()=='RISING' else 0)
        #DP.addFloat(self.piezo[0]) # DAC output min
        #DP.addFloat(self.piezo[-1]) # DAC output max
        #DP.addFloat(self.piezo[-1]) # start approach to lock point from this voltage
        self.parent.MCU.append_write_queue(DP)
        
        self.parent.MCU.mode = 'stop'
        while len(self.parent.MCU.write_queue)>0:
            pass
        time.sleep(2)
        # if something is in the read queue, drop it
        while len(self.parent.MCU.read_queue)>0:
            self.parent.MCU.pop_read_queue()
        # send waypoints
        DP = DataPackage()
        while(len(waypoints)>0):
            next_waypoint = waypoints.popleft()
            DP.addFloat(float(next_waypoint[0]))
            DP.addFloat(float(next_waypoint[1]))
        print(len(waypoints))
        self.parent.MCU.transfer(sendbytes=8*no_waypoints, output=DP, readfloats=0)
        
        self.parent.MCU.mode = 'run'
        
    def change_offset(self):
        offset = float(self.Config.lock_offset.get())
        self.scan_plot.hline(offset)
        self.scan_plot.mark(analyzescan.get_lock_points(self.piezo, self.transm, self.errors, offset), offset)
        
        
        
class CalBox:
    def __init__(self, parent, x, y, dac_id=2):
        self.parent = parent
        calframe = tk.Frame(parent, background='black', padx=3, pady=3)
        calframe.place(anchor=tk.NW, x=x, y=y)
        tk.Label(calframe, text=('Calibration DAC %d' % dac_id), anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0, column=0, columnspan=2)
        # timestamp
        self.lin_timestamp = tk.Label(calframe, text="", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.W)
        self.lin_timestamp.grid(row=1, column=0, columnspan=2)
        # make figure
        fig_lin = plt.Figure(figsize=(3.5,2))
        self.ax_lin = fig_lin.add_axes([0.2, 0.3, 0.7, 0.6])
        self.ax_lin.set_xlabel("AOM Input (V)")
        self.ax_lin.set_ylabel("Photo Diode (V)")
        self.graph_lin = FigureCanvasTkAgg(fig_lin, master=calframe)
        self.graph_lin.get_tk_widget().grid(row=2, column=0, columnspan=2)
        # buttons
        tk.Button(calframe, text="Delete Calibration", command=self.delete_lin, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=3, column=0)
        tk.Button(calframe, text="Calibrate Now", command=self.linearize, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=3, column=1)
        self.redraw_lin()
            
    def delete_lin(self):
        self.parent.Config.lin_timestamp.set('None')
        self.parent.Config.lin_pivots.set('None')
        self.redraw_lin()
        
    def linearize(self):
        LinearizeWindow(self)
        
    def redraw_lin(self):
        if self.parent.Config.lin_pivots.get() == 'None':
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
            x = self.parent.Config.GetLinPivots()[:,0]
            y = self.parent.Config.GetLinPivots()[:,1]
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
        self.lin_timestamp.config(text=('Last Calibration: '+self.parent.Config.lin_timestamp.get()))
        
        
class FFTCorrectionBox:
    def __init__(self, parent, x, y, pid_id=1, name='FFTCorrection  '):
        self.pid_id = pid_id
        self.parent = parent
        # pid frame
        pidframe = tk.Frame(parent, background='black', padx=3, pady=3)
        pidframe.place(anchor=tk.NW, x=x, y=y)
        tk.Label(pidframe, text=name, anchor=tk.W, font=('Broadway', 18), foreground='white', background='black').grid(row=0,column=0,columnspan=2)
        # k_p
        tk.Label(pidframe, text="samplerate", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=1, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=100000.0, increment=1000, borderwidth=0, textvar=self.parent.Config.kp, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=1, column=1)
        # k_i
        tk.Label(pidframe, text="batchsize", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=2, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=10000.0, increment=1000, borderwidth=0, textvar=self.parent.Config.ki, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=2, column=1)
        # k_d
        tk.Label(pidframe, text="nrOfResonantPeaks", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', anchor=tk.E).grid(row=3, column=0)
        tk.Spinbox(pidframe, from_=0.0, to=10.0, increment=1, borderwidth=0, textvar=self.parent.Config.kd, width=6, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').grid(row=3, column=1)
        # buttons
        tk.Button(pidframe, text="Tune...", command=self.auto_pid_settings, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=4, column=0)
        tk.Button(pidframe, text="Send", command=self.send_fftcorrection_settings, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').grid(row=4, column=1)
                
    def send_fftcorrection_settings(self):
        if logging:
            print("SEND FFTCorrection")
        # prepare data package to send to microcontroller and add it to the write queue
        DP = DataPackage()
        DP.addUInt8(Command_SetFFTCorrectionParameters)
        #DP.addUInt8(self.pid_id)
        DP.addFloat(float(self.parent.Config.kp.get()))
        DP.addFloat(float(self.parent.Config.ki.get()))
        DP.addFloat(float(self.parent.Config.kd.get()))
        self.parent.MCU.append_write_queue(DP)
        # write changes to config
        self.parent.Config.Save()
        
    def auto_pid_settings(self):
        if logging:
            print("AUTO PID")
        AutoPIDWindow(self)
