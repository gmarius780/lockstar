import asyncio

from spi import *

# Constants
lock_star_ip = '192.168.0.13'
lock_star_port = 10785

matrix_commands = [Command_SetClockRate, Command_SetFFTCorrectionParameters, Command_SetChannelEventVoltage]
#matrix_commands = [Command_SetChannelEventVoltage]


async def handle_new_matrix_data(reader, writer):
    data = await reader.readline()
    message = data.decode()
    addr = writer.get_extra_info('peername')
    dataArray = message.split()
    print(f"Received {message!r} from {addr!r}")

    MCU = MCU_Handler(None, speed=20000000)

    for command in matrix_commands:
        print("Command: " + str(command))
        DP = DataPackage()
        DP.addUInt8(command)  # RPi_Command_...
        
        DP_Large = DataPackage()

        if command == Command_SetClockRate:
            DP.addUInt32(int(dataArray.pop(0)))  # samplingRate
        elif command == Command_SetFFTCorrectionParameters:
            activateFFTCorrection = int(dataArray.pop(0))
            DP.addUInt8(activateFFTCorrection)  # activateFFTCorrection
            if activateFFTCorrection == 1:
                DP.addFloat(float(dataArray.pop(0)))  # batchSize
                nrOfResoanantPeaks = int(dataArray.pop(0))
                DP.addUInt32(nrOfResoanantPeaks)  # nrOfResoanantPeaks
                for i in range(0, nrOfResoanantPeaks):
                    DP_Large.addFloat(float(dataArray.pop(0)))  # freq
                    DP_Large.addFloat(float(dataArray.pop(0)))  # fftCoeffs
                    DP_Large.addFloat(float(dataArray.pop(0)))  # amplitudeShift
                    DP_Large.addFloat(float(dataArray.pop(0)))  # phaseShift
        elif command == Command_SetChannelEventVoltage:
            activateChannelVolatges = int(dataArray.pop(0))
            #DP.addUInt8(activateChannelVolatges)  # activateChannelVolatges
            if activateChannelVolatges == 1:
                channel_1_ValuesNum = int(dataArray.pop(0))
                channel_2_ValuesNum = int(dataArray.pop(0))
                DP.addUInt32(channel_1_ValuesNum)  # channel_1_ValuesNum
                DP.addUInt32(channel_2_ValuesNum)  # channel_2_ValuesNum
                channel_1_PIDNotActiveTimesNum = int(dataArray.pop(0))
                channel_2_PIDNotActiveTimesNum = int(dataArray.pop(0))
                for i in range(0, channel_1_ValuesNum):
                    DP_Large.addFloat(float(dataArray.pop(0)))  # time
                    DP_Large.addFloat(float(dataArray.pop(0)))  # vlotage
                for i in range(0, channel_2_ValuesNum):
                    DP_Large.addFloat(float(dataArray.pop(0)))  # time
                    DP_Large.addFloat(float(dataArray.pop(0)))  # vlotage
                DP.addUInt32(channel_1_PIDNotActiveTimesNum)  # channel_1_PIDNotActiveTimesNum
                DP.addUInt32(channel_2_PIDNotActiveTimesNum)  # channel_2_PIDNotActiveTimesNum
                for i in range(0, channel_1_PIDNotActiveTimesNum):
                    DP_Large.addFloat(float(dataArray.pop(0)))  # start
                    DP_Large.addFloat(float(dataArray.pop(0)))  # end
                for i in range(0, channel_2_PIDNotActiveTimesNum):
                    DP_Large.addFloat(float(dataArray.pop(0)))  # start
                    DP_Large.addFloat(float(dataArray.pop(0)))  # end


        MCU.transfer(output=DP)
        print("waiting for pin")
        while not MCU.get_GPIO_pin():
            pass
        print("pin is high!")

        if len(DP_Large.bytes) > 0:
            print("send large DP")
            MCU.transfer(output=DP_Large, sendbytes=len(DP_Large.bytes), readfloats=1)
        else:
            MCU.transfer(output=None, sendbytes=0, readfloats=1)

        print("waiting for pin")
        while not MCU.get_GPIO_pin():
            pass
        print("pin is high!")

        MCU.pop_read_queue()
        data = MCU.pop_read_queue()
        print(data)

async def main():
    server = await asyncio.start_server(
        handle_new_matrix_data, lock_star_ip, lock_star_port, limit=1024 * 1024)

    addr = server.sockets[0].getsockname()
    print(f'Serving on {addr}')

    async with server:
        await server.serve_forever()

asyncio.run(main())


