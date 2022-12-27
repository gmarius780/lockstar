from spi import *

Command_SetChannelEventVoltage = 35

MCU = MCU_Handler(None, speed=20000000)

DP = DataPackage()
DP.addUInt8(Command_SetChannelEventVoltage)  # RPi_Command
DP.addUInt8(3)  # nrOfTimeStepVoltages

"""MCU.append_write_queue(DP)
MCU.transfer(DP)

print("waiting for pin")
while not MCU.get_GPIO_pin():
    pass
print("pin is high!")

MCU.transfer(sendbytes=0, readfloats=1)
data = MCU.pop_read_queue()

data = MCU.pop_read_queue()
print(data)"""




#MCU.append_write_queue(DP)
MCU.transfer(output=DP)

print("waiting for pin")
while not MCU.get_GPIO_pin():
    pass
print("pin is high!")

#while len(MCU.read_queue)>0:
#    MCU.pop_read_queue()
    

DP = DataPackage()
DP.addFloat(1)  # time-step-1
DP.addFloat(1)  # voltage-1
DP.addFloat(2)  # time-step-2
DP.addFloat(2)  # voltage-2
DP.addFloat(4)  # time-step-3
DP.addFloat(3)  # voltage-3

MCU.transfer(output=DP, sendbytes=len(DP.bytes), readfloats=1)

"""print("waiting for pin")
while not MCU.get_GPIO_pin():
    pass
print("pin is high!")"""

#MCU.transfer(sendbytes=0, readfloats=1)

data = MCU.pop_read_queue()
#print(data)
data = MCU.pop_read_queue()
print(data)
