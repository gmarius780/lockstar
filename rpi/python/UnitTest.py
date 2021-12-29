# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

# necessary python packages
import tkinter as tk
from collections import deque

# lockstar pacakges
from scope import *
from controlgui import ControlGUI
from macros import *
from plotscan import PlotScan
from spi import *
from settings import Settings


class UnitTest(tk.Tk):
    def __init__(self):
        tk.Tk.__init__(self)
        self.MCU = MCU_Handler(self, speed=20000000, verbose=True)
        self.winfo_toplevel().title("LockStar Self Test")
        self.winfo_toplevel().config(bg='white')
        self.geometry("600x800")
        self.resizable(False, True)
        self.iconphoto(False, tk.PhotoImage(file='guitar.png'))
        self.protocol("WM_DELETE_WINDOW", self.close)
        
        # text elements
        tk.Label(self, text="Unit Self Test:\n", font=('Monospace Bold', 14), foreground='black', background='white').pack()
        self.MCU_comm = tk.Label(self, text="Testing communication to microcontroller... ", font=('Monospace Bold', 12), foreground='black', background='white')
        self.MCU_comm.pack()
        self.ADC_comm = tk.Label(self, text="Testing communication to ADC... ", font=('Monospace', 12), foreground='silver', background='white')
        self.ADC_comm.pack()
        self.DAC1_comm = tk.Label(self, text="Testing communication to DAC 1... ", font=('Monospace', 12), foreground='silver', background='white')
        self.DAC1_comm.pack()
        self.DAC2_comm = tk.Label(self, text="Testing communication to DAC 2... ", font=('Monospace', 12), foreground='silver', background='white')
        self.DAC2_comm.pack()
        
        self.update()
        
        self.run_self_test()
        
    def run_self_test(self):
        print("Going into test mode")
        # bring MCU into self-test mode
        DP = DataPackage()
        DP.addUInt8(Command_SelfTest)
        self.MCU.append_write_queue(DP)
        self.MCU.stop_live_feed()
        
        # check MCU communication by sending 2 floats and receiving them back
        print("Check communication to microcontroller")
        numbers = [1.0, 37.0]
        DP = DataPackage()
        DP.addFloat(numbers[0])
        DP.addFloat(numbers[1])
        print("  waiting for ready signal")
        while not self.MCU.get_GPIO_pin():
            pass
        self.MCU.transfer(output=DP, sendbytes=8, readfloats=0)
        print("  data sent")
        while self.MCU.get_GPIO_pin():
            pass
        print("  data received by microcontroller")
        while not self.MCU.get_GPIO_pin():
            pass
        print("  receiving data back")
        self.MCU.transfer(sendbytes=0, readfloats=2)
        while self.MCU.get_GPIO_pin():
            pass
        numbers_returned = self.MCU.pop_read_queue()
        if numbers_returned[0]==numbers[0] and numbers_returned[1]==numbers[1]:
            print("  ok")
            self.MCU_comm.config(text="Testing communication to microcontroller... ok", font=('Monospace'), foreground='green')
        else:
            print("  data wrong")
            self.MCU_comm.config(text="Testing communication to microcontroller... failed", font=('Monospace'), foreground='red')
        self.update()
        
        # check ADC communication
        print("\nCheck communication to ADC")
        print("  waiting for info")
        while not self.MCU.get_GPIO_pin():
            pass
        self.MCU.transfer_bytes(readbytes=6)
        while self.MCU.get_GPIO_pin():
            pass
        numbers_returned = self.MCU.pop_read_queue()
        print("  data received:" + str(numbers_returned))
        if (numbers_returned[2]&0b1111)==0b0111 and (numbers_returned[5]&0b1111)==0b1111:
            print("  data ok")
            self.ADC_comm.config(text="Testing communication to ADC... ok", font=('Monospace'), foreground='green')
        else:
            print("  data wrong")
            self.ADC_comm.config(text="Testing communication to ADC... failed", font=('Monospace'), foreground='red')
        self.update()
        
        # check DAC 1 communication
        print("\nCheck communication to DAC 1")
        print("  waiting for info")
        while not self.MCU.get_GPIO_pin():
            pass
        self.MCU.transfer_bytes(readbytes=4)
        while self.MCU.get_GPIO_pin():
            pass
        numbers_returned = self.MCU.pop_read_queue()
        print("  data received:" + str(numbers_returned))
        #if (numbers_returned[2]&0b1111)^0b1000 and (numbers_returned[5]&0b1111)^0b0000:
        #    print("  data ok")
        #    self.ADC_comm.config(text="Testing communication to ADC... ok", font=('Monospace'), foreground='green')
        #else:
        #    print("  data wrong")
        #    self.ADC_comm.config(text="Testing communication to ADC... failed", font=('Monospace'), foreground='red')
        self.update()
        
        # check DAC 2 communication
        print("\nCheck communication to DAC 2")
        print("  waiting for info")
        while not self.MCU.get_GPIO_pin():
            pass
        self.MCU.transfer_bytes(readbytes=4)
        while self.MCU.get_GPIO_pin():
            pass
        numbers_returned = self.MCU.pop_read_queue()
        print("  data received:" + str(numbers_returned))
        #if (numbers_returned[2]&0b1111)^0b1000 and (numbers_returned[5]&0b1111)^0b0000:
        #    print("  data ok")
        #    self.ADC_comm.config(text="Testing communication to ADC... ok", font=('Monospace'), foreground='green')
        #else:
        #    print("  data wrong")
        #    self.ADC_comm.config(text="Testing communication to ADC... failed", font=('Monospace'), foreground='red')
        self.update()
        
        self.MCU.start_live_feed()
        
        
    def close(self):
        self.MCU.stop_event.set()
        self.destroy()
        

UnitTest().mainloop()
