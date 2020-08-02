import spidev
import time
import matplotlib.pyplot as plt
import numpy as np

spi = spidev.SpiDev()
spi.open(0,0)
spi.max_speed_hz = 20000000
spi.mode = 0b00

if False:

	package_sizes = np.linspace(1,1001,11,dtype=int)#[1,3,10,30,100,300,1000,3000]
	times = np.zeros(len(package_sizes))
	for j in range(100):
		for n,package_size in enumerate(package_sizes):
			start_time = time.time()
			for i in range(1000):
				inp = spi.readbytes(package_size)
			end_time = time.time()
			times[n] += (end_time-start_time)/100000

	print(times)

	plt.plot(package_sizes, times)
	plt.show()

if False:
	package_sizes = np.linspace(1,1001,11)
	times = np.array([3.64671493e-05, 1.10278916e-04, 1.43246734e-04, 1.93365462e-04, 2.43599751e-04, 4.48088861e-04, 5.73341277e-04, 6.57018216e-04, 7.43478758e-04, 8.18210766e-04, 8.95942910e-04])
	delay = times - 8*package_sizes/20e6
	print (delay)

	plt.plot(package_sizes, delay)
	plt.show()

start_time = time.time()
for j in range(10000):
	inp = spi.readbytes(4000)
end_time = time.time()

print((end_time-start_time)/10000)
