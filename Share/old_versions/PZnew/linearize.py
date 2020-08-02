import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import matplotlib as mpl
import time
import numpy as np
from scipy.signal import savgol_filter

from spi import *
from macros import *

class LinearizeWindow(tk.Toplevel):
    
    def __init__(self, root):
        # set up window
        tk.Toplevel.__init__(self)
        self.root = root
        # force focus on this window
        self.grab_set()
        # make sure to start the microcontroller again when the window closes
        self.protocol("WM_DELETE_WINDOW", self.close)
        
        self.setup_window()
        
    def close(self):
        self.destroy()
        
        
        
    def delete(self):
        self.root.Config.lin_timestamp.set('None')
        self.root.Config.lin_pivots.set('None')
        self.redraw()
        
    def calibrate(self):
        self.delete()
        ''' TRANSFER SECTION '''
        # send calibration command
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
        #print(self.root.MCU.read_queue)
        
        ''' ANALYSIS SECTION '''
        # photo diode voltages are given over full DAC output range
        DAC_range = DAC_RANGES[self.root.Config.DAC_Setting.get()]
        AOM_Voltage = np.linspace(DAC_range[0], DAC_range[1], 1000)
        DAC_indices = np.linspace(0, 999*1048, 1000)
        PD_Voltage = self.root.MCU.pop_read_queue()
        np.savetxt("calibration_trace.txt", PD_Voltage)
        # find maximum value
        MAX = np.max(PD_Voltage)
        if MAX>0.95*DAC_range[1]:
            tk.messagebox.showwarning(title="Small ADC Input Range", message="Consider using a different ADC\nrange or attenuating the signal.")
        # plot all data
        while len(self.ax.texts)>0:
            self.ax.texts.pop(0)
        self.ax.plot(AOM_Voltage, PD_Voltage)
        self.ax.set_xlim(DAC_range)
        self.ax.set_ylim(0, 1.2*MAX)
        # Smooth data
        filtered = savgol_filter(PD_Voltage, 501, 1)
        self.ax.plot(AOM_Voltage, filtered)
        self.graph.draw()
        # find pivots, i.e. evenly spaced points in the photo diode voltage
        
        # start MCU polling again
        self.root.MCU.mode = 'run'
        
        
    def redraw(self):
        if self.root.Config.lin_pivots.get() == 'None':
            self.ax.set_xlim(0,1)
            self.ax.set_ylim(0,1)
            while len(self.ax.lines)>0:
                self.ax.lines.pop(0)
            while len(self.ax.collections)>0:
                self.ax.collections.pop(0)
            self.ax.text(0.5, 0.5, "No calibration yet", ha='center', va='center', c='C3')
            self.ax.grid(False)
        else:
            x = self.root.Config.GetLinPivots()[:,0]
            #TwosComplement2Float(x, DAC_RANGES[self.Config.DAC_Setting.get()])
            y = self.root.Config.GetLinPivots()[:,1]
            self.ax.scatter(x, y, c='C3', s=10)
            self.ax.grid()
        self.graph.draw()
        self.timestamp.config(text=('Last Calibration: '+self.root.Config.lin_timestamp.get()))
        
    def setup_window(self):
        self.winfo_toplevel().title("Setup of AOM Linearization")
        self.winfo_toplevel().config(bg='white')
        frame = tk.Frame(self, background='black')
        frame.pack(padx=3, pady=3)
        #self.geometry("700x400")
        self.resizable(False, False)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        
        # time stamp
        self.timestamp = tk.Label(frame, text=('Last Calibration: '+self.root.Config.lin_timestamp.get()), font=('Copperplate Gothic Bold', self.root.fontsize), foreground='white', background='black', anchor=tk.E)
        self.timestamp.grid(row=0, column=0, columnspan=2, pady=10)
        
        # figure
        fig = plt.Figure(figsize=(6,3.5))
        # ax 1: inputs
        self.ax = fig.add_axes([0.15, 0.15, 0.8, 0.8])
        self.ax.set_ylabel("Photo Diode Voltage (V)")
        self.ax.set_xlabel("AOM Input Voltage (V)")
        self.graph = FigureCanvasTkAgg(fig, master=frame)
        self.graph.get_tk_widget().grid(row=1, column=0, columnspan=2)
        self.redraw()
        
        tk.Button(frame, text="Delete Calibration", command=self.delete, font=('Copperplate Gothic Bold', self.root.fontsize), foreground='white', background='black').grid(row=2, column=0, pady=10)
        tk.Button(frame, text="Calibrate Now", command=self.calibrate, font=('Copperplate Gothic Bold', self.root.fontsize), foreground='white', background='black').grid(row=2, column=1, pady=10)
