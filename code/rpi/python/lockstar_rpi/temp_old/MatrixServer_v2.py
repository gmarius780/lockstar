import asyncio
import sys

from spi import *
#from controlgui import ControlGUI

async def handle_new_matrix_data(reader, writer):
    data = await reader.readline()
    message = data.decode()
    addr = writer.get_extra_info('peername')
    
    dataArray = message.split()
    
    print(f"Received {message!r} from {addr!r}")
    
    MCU = MCU_Handler(None, speed=20000000)

    DP = DataPackage()
    DP.addUInt8(int(dataArray[0]))
    dataArray.pop(0)
    DP.addUInt8(int(float(dataArray[0])))
    dataArray.pop(0)
    
    MCU.transfer(output=DP)

    print("waiting for pin")
    while not MCU.get_GPIO_pin():
        pass
    print("pin is high!")
        
    DP = DataPackage()
    for item in dataArray:
        DP.addFloat(float(item))

    MCU.transfer(output=DP, sendbytes=len(DP.bytes), readfloats=1)

    MCU.pop_read_queue()
    data = MCU.pop_read_queue()
    print(data)
    

async def main():
    server = await asyncio.start_server(
        handle_new_matrix_data, '192.168.0.12', 10785, limit=1024 * 1024)

    addr = server.sockets[0].getsockname()
    print(f'Serving on {addr}')

    async with server:
        await server.serve_forever()

# print("Python Version:", sys.version)
asyncio.run(main())