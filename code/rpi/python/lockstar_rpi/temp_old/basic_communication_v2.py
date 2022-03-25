from spi import *

Command_SetChannelEventVoltage = 35

MCU = MCU_Handler(None, speed=20000000)

# send command to MCU
DP = DataPackage()
DP.addUInt8(Command_SetChannelEventVoltage)
DP.addUInt8(2)

MCU.transfer(output=DP)

print("waiting for pin")
while not MCU.get_GPIO_pin():
    pass
print("pin is high!")

DP = DataPackage()
DP.addFloat(1)  # time-step-1
DP.addFloat(1)  # voltage-1
DP.addFloat(2)  # time-step-2
DP.addFloat(2)  # voltage-2

MCU.transfer(output=DP, sendbytes=4*2*2)

print("waiting for pin")
while not MCU.get_GPIO_pin():
    pass
print("pin is high!")

MCU.transfer(sendbytes=0, readfloats=1)

data = MCU.pop_read_queue()
data = MCU.pop_read_queue()
print(data)