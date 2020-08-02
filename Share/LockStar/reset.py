#!/usr/bin/python

import RPi.GPIO as GPIO
import time

def reset():
	GPIO.setmode(GPIO.BCM)
	GPIO.setup(7, GPIO.OUT)
	GPIO.output(7, GPIO.LOW)
	time.sleep(0.1)
	GPIO.setup(7, GPIO.IN)
	return



def main():
	reset()
	return 0


if __name__ == '__main__':
	main()
