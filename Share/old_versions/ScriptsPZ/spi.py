#!/usr/bin/python

import time
import spidev
import sys
from struct import *


def ReadFloatChunk(spi,length):
	floats_left = length
	spiinput = []
	while floats_left>0:
		temp_len = min(4*floats_left,4096)
		spiinput.extend(spi.readbytes(temp_len))
		floats_left-=1024
		
	fmt = str(int(length)) + 'f'
	values = unpack(fmt, bytes(spiinput))
	
	return list(values)
	
	
def ReadUInt16Chunk(spi,length):
	ints_left = length
	spiinput = []
	while ints_left>0:
		temp_len = min(2*ints_left,4096)
		spiinput.extend(spi.readbytes(temp_len))
		ints_left-=2048
		
	repr_values = ''.join(chr(byte) for byte in spiinput)
	fmt = str(int(length)) + 'H'
	values = unpack(fmt, repr_values)
	
	return list(values)

def ReadInt16Chunk(spi,length):
	ints_left = length
	spiinput = []
	while ints_left>0:
		temp_len = min(2*ints_left,4096)
		spiinput.extend(spi.readbytes(temp_len))
		ints_left-=2048
		
	repr_values = ''.join(chr(byte) for byte in spiinput)
	fmt = str(int(length)) + 'h'
	values = unpack(fmt, repr_values)
	
	return list(values)
	
def ReadFloat(spi):
	spiinput = spi.readbytes(4)
	fmt = 'f'
	value = unpack(fmt, bytes(spiinput))
	return value[0]
	
def ReadUInt16(spi):
	spiinput = spi.readbytes(2)
	repr_value = ''.join(chr(byte) for byte in spiinput)
	fmt = 'H'
	value = unpack(fmt, repr_value)
	return value[0]

def ReadInt16(spi):
	spiinput = spi.readbytes(2)
	repr_value = ''.join(chr(byte) for byte in spiinput)
	fmt = 'h'
	value = unpack(fmt, repr_value)
	return value[0]
	
	
	
def WriteFloat(spi,val):
	i = unpack('BBBB', pack('f', val))
	spi.writebytes(list(i))
	return 0 
	
def WriteUInt32(spi,val):
	i = unpack('BBBB', pack('I', val))
	spi.writebytes(list(i))
	return 0 
	
def WriteUInt16(spi,val):
	i = unpack('BB', pack('H', val))
	spi.writebytes(list(i))
	return 0 
                
def WriteFloatChunk(spi,lst):
	l = len(lst)
	fmt1 = str(int(l)) + 'f'
	fmt2 = str(4*int(l)) + 'B'	
	spioutput = (list(unpack(fmt2, pack(fmt1, *lst))))
	i = 0
	while l>0:
		temp_len = min(4*l,4096)
		spi.writebytes(spioutput[i*4096:i*4096+temp_len])
		l-=1024
		i+=1
		time.sleep(0.1) # Abhaening von SPI frequency! Current: 500khz
	return 0
	
def WriteUInt16Chunk(spi,lst):
	l = len(lst)
	fmt1 = str(int(l)) + 'H'
	fmt2 = str(2*int(l)) + 'B'	
	spioutput = (list(unpack(fmt2, pack(fmt1, *lst))))
	i = 0
	while l>0:
		temp_len = min(2*l,4096)
		spi.writebytes(spioutput[i*4096:i*4096+temp_len])
		l-=2048
		i+=1
		time.sleep(0.1) # Abhaengig von SPI frequency! Current: 500khz
	return 0
	
def SetupSPI(speed=5000000):
	spi = spidev.SpiDev()
	spi.open(0,0)
	spi.max_speed_hz = speed
	spi.mode = 0b00
	return spi


if __name__ == '__main__':
	SetupSPI()
