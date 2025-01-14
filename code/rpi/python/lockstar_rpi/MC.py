from lockstar_rpi.BackendSettings import BackendSettings
import asyncio
import logging
from struct import pack, unpack, calcsize # https://docs.python.org/3/library/struct.html
from lockstar_rpi.MCDataPackage import MCDataPackage
from math import ceil
from time import sleep, perf_counter

if not BackendSettings.debug_mode:
    try:
        from spidev import SpiDev
        import RPi.GPIO as GPIO
    except Exception as ex:
        logging.error(f'cannot import rpi packags: {ex}')

class MC:
    """Represents the Microcontroller: Sends and receives MCDataPackage's to/from the MC
    """
    __instance = None


    @staticmethod
    def I():
        if MC.__instance is None:
            MC.__instance = MC()
        return MC.__instance

    def __init__(self) -> None:
        self._rpi_lock = asyncio.Lock()

        # check if it is running on a raspberry-pi (not debug mode)
        if not BackendSettings.debug_mode:
            # set up SPI
            try:
                self._spi = SpiDev()
                self._spi.open(0,0)
                self._spi.max_speed_hz = BackendSettings.mc_communication_speed_Hz
                self._spi.mode = 0b00
            except Exception as ex:
                logging.error(f"MC: Cannot set up SPI: {ex}")
                raise ex
            # set up GPIO pin
            try:
                GPIO.setmode(GPIO.BCM)
                GPIO.setup(BackendSettings.mc_gpio_input_channel, GPIO.IN)
            except Exception as ex:
                logging.error(f'MC: Cannot setup GPIO pin: {ex}')
                raise ex
        else:
            # TODO: MELVIN: you can put your 'relay code here' (if it is possible to leave the other methods the same, this would be perfect, otherwise
            # we might have to create a subclass, implementing different read/write methods))
            pass
    

    #=== READ METHODS

    async def read_ack(self, timeout_s=1):
        """Reads two bools. if both are 1, this is interpreted as an ACK (signalling success of whatever happened before)
            
            :Return True if ack False otherwise
        """
        #unpack data
        start_time = perf_counter()
        exception = ''
        payload_length = -1
        raw_data = ''
        while (timeout_s is None or perf_counter() - start_time < timeout_s):
            if timeout_s is None:
                timeout_s = 0
            try:
                if self.get_GPIO_pin() == True: # wait for rising flank of GPIO pin
                    logging.debug('gpio high')
                    payload_length, raw_data = await self.read_mc_data()
                    return MCDataPackage.pop_ack_nack_from_buffer(bytes(raw_data))
                else:
                    logging.debug('gpio low')
                    sleep(0.05) #leave the MC time to work
            except Exception as ex:
                exception = ex
        logging.error(f'MC.read_mc_data_package: cannot unpack data: {payload_length}: {exception}: {raw_data}')
        return False

    async def read_mc_data_package(self, list_str_cpp_dtype, timeout_s=1):
        #unpack data
        start_time = perf_counter()
        exception = ''
        payload_length = -1
        raw_data = ''
        while (timeout_s is None or perf_counter() - start_time < timeout_s):
            if timeout_s is None:
                timeout_s = 0
            try:
                if self.get_GPIO_pin() == True: # wait for rising flank of GPIO pin
                    logging.debug('gpio high')
                    payload_length, raw_data = await self.read_mc_data()
                    unpacked_data = MCDataPackage.pop_from_buffer(list_str_cpp_dtype, bytes(raw_data))
                    return payload_length, unpacked_data
                else:
                    logging.debug('gpio low')
                    # sleep(0.1)
                    sleep(0.05) #leave the MC time to work
            except Exception as ex:
                exception = ex
        
        logging.error(f'MC.read_mc_data_package: cannot unpack data: {payload_length}: {exception}: {raw_data}')
        return False

    async def read_mc_data(self):
        """Reads first an unsigned int, which is interpreted as the length of the payload, which the MC reponds to the RPI

        :Return (unsigned int, byte-array): (payload length, payload)
        """
        async with self._rpi_lock:
            
            #read unsigned int corresponding to the payload size
            payload_length = None
            try:
                read_bytes = self._spi.readbytes(calcsize('<I'))
                payload_length = unpack('<I', bytes(read_bytes))[0]
            except Exception as ex:
                logging.error(f'MC.read_mc_data_package: {read_bytes} cannot parse payload length: {ex}')
            logging.info(f'MC.read_MC_data: payload_length: {payload_length}')
            if payload_length is not None and payload_length > 0 and payload_length < 100000:
                bytes_left = payload_length
                output = []
                try:
                    while bytes_left>0:
                        batchsize = min(bytes_left, BackendSettings.mc_write_buffer_size)
                        spiinput = self._spi.readbytes(batchsize)
                        output += list(spiinput)
                        bytes_left -= batchsize
                except Exception as ex:
                    logging.error(f'MC.read_mc_data_package: cannot read payload of length: {payload_length}: {ex}')

                return payload_length, output

            else:
                logging.error(f'MC.read_MC_data: payload_length invalid: {payload_length}')
                return 0, []


    #=== WRITE METHODS 
    async def write_mc_data_package(self, mc_data_package):
        try:
            logging.info(f'nbr of bytes: {mc_data_package.get_nbr_of_bytes()}')
            await self.initiate_communication(mc_data_package.get_nbr_of_bytes())
            sleep(0.05)#sleep to wait for the mc to start DMA (needed)
        except Exception as ex:
            logging.error(f'MC:write_mc_data_package: invalid data package: {ex}')

        #fill up bytes such that len is a multiple of READ_NBR_BYTES_MULTIPLIER, because MC expects multiples of MC.READ_NBR_BYTES_MULTIPLIER
        arr_bytes = mc_data_package.get_bytes()
        if len(arr_bytes) % BackendSettings.MC_READ_NBR_BYTES_MULTIPLIER != 0:
            arr_bytes += bytes((BackendSettings.MC_READ_NBR_BYTES_MULTIPLIER*ceil(len(arr_bytes)/BackendSettings.MC_READ_NBR_BYTES_MULTIPLIER) - len(arr_bytes))*[0])
        await self.write(arr_bytes)

    async def initiate_communication(self, nbr_of_bytes_to_send):
        """Sends one byte via SPI to the MC. The value of the bytes tells the MC how many 'tens-of-bytes' it should expect via DMA

        Args:
        :param    tens_of_bytes_to_read (int): dens of bytes to expect for the MC via DMA
        """
        await self.write(pack('<B', ceil(nbr_of_bytes_to_send/BackendSettings.MC_READ_NBR_BYTES_MULTIPLIER)))

    async def write(self, arr_bytes):
        async with self._rpi_lock:
            try:
                self._spi.writebytes2(arr_bytes)
                logging.info(f'write stuff to MC:{arr_bytes if len(arr_bytes) < 50 else arr_bytes[0:50]}')
            except Exception as ex:
                logging.error(f'MC: Cannot send bytes to rpi: {ex}. len-bytes: {len(arr_bytes)}')


    #=== GPIO PIN
    def get_GPIO_pin(self):
        try:
            return GPIO.input(BackendSettings.mc_gpio_input_channel)
        except:
            print("GPIO Issue.")
            raise NameError('No GPIO')

    def set_GPIO_pin_high(self):
        try:
            return GPIO.output(BackendSettings.mc_gpio_input_channel, GPIO.HIGH)
        except Exception as ex:
            print(f'GPIO Issue: {ex}')
            raise NameError('No GPIO')
        
    def set_GPIO_pin_low(self):
        try:
            return GPIO.output(BackendSettings.mc_gpio_input_channel, GPIO.LOW)
        except Exception as ex:
            print(f'GPIO Issue: {ex}')
            raise NameError('No GPIO')


if __name__ == "__main__":
    import sys
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )

    if len(sys.argv) > 1:
        if sys.argv[1] == 'lock':
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
            asyncio.run(MC.I().write_mc_data_package(mc_data_package))
            print(asyncio.run(MC.I().read_ack()))

        elif sys.argv[1] == 'unlock':
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
            asyncio.run(MC.I().write_mc_data_package(mc_data_package))
            print(asyncio.run(MC.I().read_ack()))
        elif sys.argv[1] == 'setpid':
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
            mc_data_package.push_to_buffer('float', float(32.3)) # p
            mc_data_package.push_to_buffer('float', float(88.9)) # i
            mc_data_package.push_to_buffer('float', float(99.6)) # d
            asyncio.run(MC.I().write_mc_data_package(mc_data_package))
            print(asyncio.run(MC.I().read_ack()))

    else:
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        print(mc_data_package.get_nbr_of_bytes())

