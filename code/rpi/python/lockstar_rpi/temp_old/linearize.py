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

from spi import *
from macros import *

fontsize = 12

class LinearizeWindow(tk.Toplevel):
    
    def __init__(self, root):        
        # set up window
        tk.Toplevel.__init__(self)
        self.root = root
        # force focus on this window
        self.grab_set()
        # make sure to start the microcontroller again when the window closes
        self.protocol("WM_DELETE_WINDOW", self.close)
        # delete previous calibration
        self.root.delete_lin()
        
        # set up variables
        self.clipmax = tk.StringVar(self, "0.95")
        self.sg_wl = tk.StringVar(self, "101")
        self.sg_po = tk.StringVar(self, "7")
        self.data = None
        
        self.PD_voltage = None
        
        self.setup_window()
        
        self.calibrate()
        
        for stringvar in [self.clipmax, self.sg_wl, self.sg_po]:
            stringvar.trace('w', lambda name, index, mode, sv=stringvar: self.calculate())
        
    def close(self):
        # close window
        self.destroy()
        
        
    def calibrate(self):
        ''' TRANSFER SECTION '''
        # send calibration command
        DP = DataPackage()
        DP.addUInt8(Command_MeasureAOMResponse)
        DP.addUInt8(2) # ADC Channel
        DP.addUInt8(2) # DAC Channel
        #M:self.root.parent.MCU.append_write_queue(DP)
        self.root.parent.MCU.append_write_queue(DP)
        
        # interrupt live data for upcoming transfer
        #M:self.root.parent.MCU.stop_live_feed()
        self.root.parent.MCU.stop_live_feed()
        
        # with the next rising flank on the GPIO, calibration data is ready
        #M:while not self.root.parent.MCU.get_GPIO_pin():
        while not self.root.parent.MCU.get_GPIO_pin():
            pass
        # read data from MCU
        #M:self.root.parent.MCU.transfer(sendbytes=0, readfloats=1000)
        self.root.parent.MCU.transfer(sendbytes=0, readfloats=1000)
        
        # restart live data
        #M:self.root.parent.MCU.start_live_feed()
        self.root.parent.MCU.start_live_feed()
        
        ''' ANALYSIS SECTION '''
        # photo diode voltages are given over full DAC output range
        DAC_range = DAC_RANGES[self.root.parent.Config.DAC_Setting.get()]
        
        #M:
        ADC_range = ADC_RANGES[self.root.parent.Config.ADC_Setting.get()]
        
        self.AOM_voltage = np.linspace(DAC_range[0], DAC_range[1], 1000)
        DAC_indices = np.linspace(0, 999*1048, 1000)
        self.PD_voltage = self.root.parent.MCU.pop_read_queue()[0:]
        #np.savetxt("calibration_trace.txt", PD_Voltage)
        #self.PD_voltage = np.genfromtxt("calibration_trace.txt")
        # find maximum value
        print(f'start pd-voltage: {self.PD_voltage[:10]}')
        print(f'dac-range: {DAC_range}')
        MAX = np.max(self.PD_voltage)
        print(f'max pd-voltage: {MAX}')
        print(f'pd-voltage shape: {len(self.PD_voltage)}')
        #M: is this really correct??if MAX>0.95*DAC_range[1]:
        if MAX>0.95*ADC_range[1]:
            tk.messagebox.showwarning(title="Small ADC Input Range", message="Consider using a different ADC\nrange or attenuating the signal.")
        
        self.calculate()

        
    def calculate(self):
        #self.filtered = savgol_filter(self.PD_voltage, int(self.sg_wl.get()), int(self.sg_po.get()))
        self.filtered = self.PD_voltage
        MAX = np.max(self.filtered)
        print("MAX: %f" % MAX)
        
        # estimate plot ranges
        tenpercent = np.argmax(self.filtered>0.1*MAX)
        print("10p: %d" % tenpercent)
        ninetypercent = np.argmax(self.filtered>0.9*MAX)
        print("90p: %d" % ninetypercent)
        distance = ninetypercent - tenpercent
        print("distance: %d" % distance)
        self.small_span = [int(np.clip(tenpercent-distance,0,999)), int(np.clip(tenpercent+0.3*distance,0,999))]
        print(self.small_span)
        self.big_span = [int(np.clip(tenpercent-distance,0,999)), int(np.clip(tenpercent+distance,0,999))]
        print(self.big_span)
        print(self.filtered)
        self.ZERO = np.nan_to_num(np.mean(self.filtered[:self.small_span[0]]))
        print(f'1: {self.ZERO}')
        LIMIT = float(self.clipmax.get())*MAX
        print(f'2: {LIMIT}')
        self.SPAN = LIMIT-self.ZERO
        print(f'3: {self.SPAN}')
        
        interpolation = interp1d(self.AOM_voltage, self.filtered)
        
        self.pivots_voltage = [root(lambda x: (interpolation(x)-self.ZERO)/self.SPAN-fraction, x0=(self.AOM_voltage[self.big_span[0]]+self.AOM_voltage[self.big_span[1]])/2).x[0] for fraction in [0.001]+list(np.linspace(1.0/LIN_NUMBER_PIVOTS, 1, LIN_NUMBER_PIVOTS-1))] #bracket=[self.AOM_voltage[self.big_span[0]], self.AOM_voltage[self.big_span[1]]]
        #self.pivots_voltage = np.linspace(1,2,101)
        self.redraw()
        
        
    def redraw(self):
        # clear plot
        for i in range(3):
            while len(self.ax[i].lines)>0:
                self.ax[i].lines.pop(0)
            while len(self.ax[i].collections)>0:
                self.ax[i].collections.pop(0)
            while len(self.ax[i].texts)>0:
                self.ax[i].texts.pop(0)
        if self.PD_voltage is None:
            for i in range(3):
                self.ax[i].set_xlim(0,1)
                self.ax[i].set_ylim(0,1)
                self.ax[i].text(0.5, 0.5, "Waiting for\ncalibration ...", ha='center', va='center', c='C3')
                self.ax[i].grid(False)
        else:
            #x = self.root.Config.GetLinPivots()[:,0]
            #TwosComplement2Float(x, DAC_RANGES[self.Config.DAC_Setting.get()])
            #y = self.root.Config.GetLinPivots()[:,1]
            self.ax[0].plot(self.AOM_voltage, self.PD_voltage, c='C0')
            self.ax[0].plot(self.AOM_voltage, self.filtered, c='C1')
            self.ax[0].plot([self.pivots_voltage[0], self.pivots_voltage[0], self.pivots_voltage[-1], self.pivots_voltage[-1], self.pivots_voltage[0]], [self.ZERO, self.ZERO+self.SPAN, self.ZERO+self.SPAN, self.ZERO, self.ZERO], c='C2', lw=0.5)
            self.ax[0].text(self.pivots_voltage[0]+0.05*(self.pivots_voltage[-1]-self.pivots_voltage[0]), self.ZERO+self.SPAN*0.95, "ROI", c='C2', va='top', ha='left')
            self.ax[0].set_xlim(self.AOM_voltage[self.big_span])
            self.ax[0].set_ylim(self.ZERO-self.SPAN/10, self.ZERO+1.1*self.SPAN)
            legend = [mpl.lines.Line2D([0], [0], color='C0', lw=4), mpl.lines.Line2D([0], [0], color='C1', lw=4)]
            self.ax[0].legend(legend, ['Raw', 'Filtered'], loc='lower right', prop={'size': 8})
            self.ax[1].plot(self.AOM_voltage, self.PD_voltage, c='C0')
            self.ax[1].plot(self.AOM_voltage, self.filtered, c='C1')
            self.ax[1].scatter(self.pivots_voltage, np.linspace(0, 1, 101)*self.SPAN+self.ZERO, c='C3', zorder=20)
            self.ax[1].hlines(np.linspace(self.ZERO, self.ZERO+self.SPAN, 101), 0, 1000, lw=0.5, color='w')
            self.ax[1].set_xlim(self.AOM_voltage[self.small_span[0]], self.pivots_voltage[12])
            self.ax[1].set_ylim(self.ZERO-self.SPAN/100, self.ZERO+self.SPAN/10)
            self.ax[1].legend(legend, ['Raw', 'Filtered'], loc='upper left', prop={'size': 8})
            self.ax[2].scatter(self.pivots_voltage, np.linspace(0, 1, 101)*self.SPAN+self.ZERO, c='C3', s=5)
            self.ax[2].set_xlim(self.AOM_voltage[self.big_span])
            self.ax[2].set_ylim(self.ZERO-self.SPAN/10, self.ZERO+1.1*self.SPAN)
        
            for i in range(3):
               self.axt[i].set_ylim(100*(self.ax[i].get_ylim()-self.ZERO)/self.SPAN)
            self.graph.draw()
            
    def accept(self):
        # save calibration
        print("pivot_voltage")
        print(self.pivots_voltage)
        #self.root.parent.Config.SetLinPivots([[self.pivots_voltage[i], self.ZERO+self.SPAN*i/LIN_NUMBER_PIVOTS] for i in range(len(self.pivots_voltage))], datetime.datetime.now().strftime("%d/%m/%Y %H:%M:%S"))
        self.root.parent.Config.SetLinPivots(np.transpose([self.pivots_voltage, self.ZERO+np.linspace(0,1,LIN_NUMBER_PIVOTS)*self.SPAN]).tolist(), datetime.datetime.now().strftime("%d/%m/%Y %H:%M:%S"))
        self.root.redraw_lin()
        # send calibration to microcontroller
        DP = DataPackage()
        DP.addUInt8(Command_ProgramCalibration)
        DP.addUInt8(2) # DAC number
        DP.addFloat(self.ZERO) # minimum
        DP.addFloat(self.ZERO+self.SPAN) # maximum
        DP.addUInt32(LIN_NUMBER_PIVOTS) # number of points
        self.root.parent.MCU.append_write_queue(DP)
        
        print(self.pivots_voltage)
        print(len(self.pivots_voltage))
        
        print("before stop")
        # interrupt live data for upcoming transfer
        self.root.parent.MCU.stop_live_feed()
        print("after stop")
        
        # with the next rising flank on the GPIO, MCU is ready to receivce
        while not self.root.parent.MCU.get_GPIO_pin():
            pass
        print("after wait")
        # make data package
        DP = DataPackage()
        print(self.pivots_voltage)
        print(len(self.pivots_voltage))
        print(LIN_NUMBER_PIVOTS)
        for i in range(LIN_NUMBER_PIVOTS):
            DP.addFloat(self.pivots_voltage[i])
        # send data to MCU
        print("before transfer")
        self.root.parent.MCU.transfer(output=DP, sendbytes=(4*LIN_NUMBER_PIVOTS), readfloats=0)
        print("after transfer")
        # restart live data
        self.root.parent.MCU.start_live_feed()
        print("after live restart")
        # close the window
        self.close()
        
    def setup_window(self):
        self.winfo_toplevel().title("Setup of AOM Linearization")
        self.winfo_toplevel().config(bg='white')
        frame = tk.Frame(self, background='black')
        frame.pack(padx=3, pady=3)
        #self.geometry("700x400")
        self.resizable(False, False)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        
        # figure
        fig, self.ax = plt.subplots(1,3,figsize=(9,3))
        # ax 1: inputs
        self.ax[0].set_ylabel("Photo Diode Voltage (V)")
        self.ax[0].set_title("Revelant AOM Range")
        self.ax[1].set_title("Lower end of AOM Range")
        self.ax[2].set_title("Resulting Pivotal Points")
        self.axt = [None]*3
        for i in range(3):
            self.ax[i].set_xlabel("AOM Input Voltage (V)")
            self.axt[i] = self.ax[i].twinx()
            self.axt[i].yaxis.set_major_formatter(mtick.PercentFormatter())
        plt.tight_layout()
        self.graph = FigureCanvasTkAgg(fig, master=frame)
        self.graph.get_tk_widget().pack()
        
        # info
        tk.Label(frame, text="Choose at which power you want to clip the calibration (it gets very flat towards 100\%).\nThen tweak the parameters of the Savitzky-Golay filter to get good agreement between raw and filtered data.", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack()
        
        # filter setup
        filterframe = tk.Frame(frame)
        filterframe.pack()
        tk.Label(filterframe, text="clip max: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(filterframe, from_=0.00, to=1.00, increment=0.01, borderwidth=0, textvar=self.clipmax, width=4, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        tk.Label(filterframe, text=" Savitzky-Golay Filter Window: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(filterframe, from_=1, to=501, increment=2, borderwidth=0, textvar=self.sg_wl, width=3, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        tk.Label(filterframe, text=" Poly Order: ", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Spinbox(filterframe, from_=1, to=20, increment=1, borderwidth=0, textvar=self.sg_po, width=2, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black', buttonbackground='black').pack(side=tk.LEFT)
        
        # info
        tk.Label(frame, text="\nYou can tweak the zero position of the AOM with these two buttons. This is a bit ambiguous.\nNote that there will be linear interpolation between this zero point and the 1\% pivot.", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack()
        
        # zero position
        zeroframe = tk.Frame(frame)
        zeroframe.pack()
        tk.Button(zeroframe, text="Shift Zero down", command=(lambda: self.shift_zero(False)), font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Button(zeroframe, text="Shift Zero up", command=(lambda: self.shift_zero(True)), font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        
        # info
        tk.Label(frame, text="\nWhen you are happy with the calibration, click Accept.", font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack()
        
        # cancel / accept 
        buttonframe = tk.Frame(frame)
        buttonframe.pack()
        tk.Button(buttonframe, text="Cancel", command=self.close, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.LEFT)
        tk.Button(buttonframe, text="Accept", command=self.accept, font=('Copperplate Gothic Bold', fontsize), foreground='white', background='black').pack(side=tk.RIGHT)
        
        self.redraw()
        
    def shift_zero(self, up):
        if up:
            self.pivots_voltage[0] += (self.pivots_voltage[-1]-self.pivots_voltage[0])/50
        else:
            self.pivots_voltage[0] -= (self.pivots_voltage[-1]-self.pivots_voltage[0])/50
        self.redraw()
