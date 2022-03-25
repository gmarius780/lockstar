# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

# necessary python packages
import tkinter as tk
from collections import deque
import matplotlib.pyplot as plt

# lockstar pacakges
from scope import *
from controlgui import ControlGUI
from macros import *
from plotscan import PlotScan
from spi import *
from settings import Settings

MCU = MCU_Handler(None, speed=20000000)

'''DP = DataPackage()
DP.addUInt8(Command_SetFFTCorrectionParameters)
DP.addFloat(10000) # record channel 1: true
DP.addFloat(2000) # record channel 2: true
DP.addUInt8(1) # output channel 2
DP.addFloat(1800) # from
DP.addFloat(1) # to
DP.addFloat(0)
DP.addFloat(0)
    
    
MCU.append_write_queue(DP)
MCU.transfer(sendbytes=4*8, readfloats=0)'''

while not MCU.get_GPIO_pin():
    pass
    
MCU.transfer(sendbytes=0, readfloats = 2)
data = MCU.pop_read_queue()

print(data)

#file_out = open("Data.txt","a") 
#for line in data:
#        file_out.write(str(line) + "\n")
#file_out.close()

# plt.plot(data)

# t = np.linspace(start=0, stop=10000, num=10000)
# s = 0.4*np.sin(2*np.pi*1000*t)

#plt.plot(s)
# plt.show()
