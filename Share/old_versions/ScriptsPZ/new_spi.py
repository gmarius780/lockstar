#!/usr/bin/python

import time
import spidev
import sys
from struct import *
from collections import deque

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
		

class SPI_Handler:
	def __init__(self, speed=1000000):
		self.spi = spidev.SpiDev()
		self.spi.open(0,0)
		self.spi.max_speed_hz = speed
		self.spi.mode = 0b00
		
	def transfer(self, output=None):
		if output==None:
			output = DataPackage()
		while len(output.bytes)<20:
			output.bytes.append(0)
		self.spi.writebytes(output.bytes)
		spiinput = self.spi.readbytes(4000)
		values = unpack('1000f', bytes(spiinput))
		return list(values)

