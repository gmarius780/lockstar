#!/usr/bin/python

import time
import sys
from collections import deque
from struct import *
import threading
import sys
try:
    import spidev
    import RPi.GPIO as GPIO
except:
    print("spidev / RPi.GPIO not installed!")
    
from macros import *
    
input_channel = 8

class DataPackage():
	def __init__(self):
		self.bytes = []
		
	def addUInt8(self, value):
		self.bytes.append(value)
		
	def addUInt16(self, value):
		self.bytes += unpack('BB', pack('H', value))
		
	def addUInt32(self, value):
		self.bytes += unpack('BBBB', pack('I', value))
		
	def addFloat(self, value):
		self.bytes += unpack('BBBB', pack('f', value))
		

class MCU_Handler:
    def __init__(self, GUI, speed=1000000):
        self.mode = "run"
        self.GUI = GUI
        # read queue
        self.read_queue = deque()
        self.read_queue_blocked = False
        # write queue
        self.write_queue = deque()
        self.write_queue_blocked = False
        # set up SPI
        try:
            self.spi = spidev.SpiDev()
            self.spi.open(0,0)
            self.spi.max_speed_hz = speed
            self.spi.mode = 0b00
        except:
            print("Cannot set up SPI: spidev not installed!")
        # set up GPIO pin
        try:
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(input_channel, GPIO.IN)
        except:
            print("Problem setting up GPIO pin.")
        # start thread that tries to poll new data from the MCU
        # in order to be able to stop the thread, we need a stop event
        self.stop_event = threading.Event()
        threading.Thread(target=self.get_data).start()
        
    def set_mode(self, mode):
        self.mode = mode
        
    def transfer(self, output=None, sendbytes=SEND_BYTES, readfloats=READ_FLOATS):
        # write data
        if sendbytes>0:
            if output==None:
                output = DataPackage()
            while len(output.bytes)<sendbytes:
                output.bytes.append(0)
            try:
                self.spi.writebytes(output.bytes)
            except:
                print("problem sending data over SPI")
        # read data
        try:
            floats_left = readfloats
            output = []
            while floats_left>0:
                batchsize = min(floats_left, 1000)
                spiinput = self.spi.readbytes(batchsize*4)
                values = unpack(str(int(batchsize))+'f', bytes(spiinput))
                output += list(values)
                floats_left -= batchsize
            self.append_read_queue(output)
        except:
            e = sys.exc_info()
            print(e)
        
    ''' The following functions deliver append/pop functionality to the read and write queues in a thread-safe way '''    
    def append_read_queue(self, data):
        while self.read_queue_blocked:
            pass
        self.read_queue_blocked = True
        self.read_queue.append(data)
        self.read_queue_blocked = False      
    def pop_read_queue(self):
        while self.read_queue_blocked:
            pass
        self.read_queue_blocked = True
        result = self.read_queue.popleft()
        self.read_queue_blocked = False
        return result
    def append_write_queue(self, data):
        while self.write_queue_blocked:
            pass
        self.write_queue_blocked = True
        self.write_queue.append(data)
        self.write_queue_blocked = False
    def pop_write_queue(self):
        while self.write_queue_blocked:
            pass
        self.write_queue_blocked = True
        result = self.write_queue.popleft()
        self.write_queue_blocked = False
        return result
    
    def get_GPIO_pin(self):
        try:
            return GPIO.input(input_channel)
        except:
            print("GPIO Issue.")
            raise NameError('No GPIO')
    
    def get_data(self):
        while True:
            if self.stop_event.is_set():
                return
            # normal running operation? (with PID+Scope)
            # also, always empty the write queue
            if self.mode == "run" or len(self.write_queue)>0:
                try:
                    # wait for input pin to go high
                    while not self.get_GPIO_pin():
                        pass
                    # transmit and receive data
                    if len(self.write_queue)>0:
                        self.transfer(self.pop_write_queue())
                    else:
                        self.transfer()
                    print("Got data!")
                    # wait for input pin to go low
                    while self.get_GPIO_pin():
                        pass
                except:
                    print("SPI / GPIO problem")
                    time.sleep(1)
            # running is stopped? Check in again later
            else:
                time.sleep(1)
            
