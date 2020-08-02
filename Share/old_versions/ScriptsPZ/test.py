
#!/usr/bin/python3

from spi import *
import numpy as np
import matplotlib.pyplot as plt
import time

samples = 10000
wait_time = 0.1+samples*1e-5
spi = SetupSPI(speed=5000000);

a = ReadFloat(spi)
print(a)
