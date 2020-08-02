#!/usr/bin/python3

from spi import *
import numpy as np
import matplotlib.pyplot as plt
import time

samples = 10000
wait_time = 0.1+samples*1e-5
spi = SetupSPI(speed=5000000);

print("Benchmarking Analog Performance\n")

nothing = input("Please terminate Input 1 and confirm with Enter.")

WriteUInt16(spi, 0)

time.sleep(wait_time)

Input1_0V = ReadFloatChunk(spi, samples)
np.savetxt("input1_0V.txt", Input1_0V)
print(np.mean(Input1_0V))
plt.plot(Input1_0V)
plt.show()



nothing = input("Please terminate Input 2 and confirm with Enter.")


WriteUInt16(spi, 0)

time.sleep(wait_time)

Input2_0V = ReadFloatChunk(spi, samples)
np.savetxt("input2_0V.txt", Input2_0V)
print(np.mean(Input2_0V))
plt.plot(Input2_0V)
plt.show()


nothing = input("Please set Input 1 to 9V and confirm with Enter.")

WriteUInt16(spi, 0)

time.sleep(wait_time)

Input1_9V = ReadFloatChunk(spi, samples)
np.savetxt("input1_9V.txt", Input1_9V)
print(np.mean(Input1_9V))
plt.plot(Input1_9V)
plt.show()




nothing = input("Please set Input 2 to 9V and confirm with Enter.")

WriteUInt16(spi, 0)

time.sleep(wait_time)

Input2_9V = ReadFloatChunk(spi, samples)
np.savetxt("input2_9V.txt", Input2_9V)
print(np.mean(Input2_9V))
plt.plot(Input2_9V)
plt.show()

