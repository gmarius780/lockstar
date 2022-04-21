from lockstar_rpi.BackendSettings import BackendSettings
from spidev import SpiDev
import RPi.GPIO as GPIO
import asyncio
import logging
from struct import pack, unpack, calcsize # https://docs.python.org/3/library/struct.html

class MC:
    """Represents the Microcontroller: Sends and receives MCDP's to/from the MC
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
    
    async def read_ack(self):
        """Reads two bools. if both are 1, this is interpreted as an ACK (signalling success of whatever happened before)
            
            Returns: True if ack False otherwise
        """
        async with self._rpi_lock:
            try:
                (r1, r2) = unpack('>??', self.spi.readbytes(calcsize('>??')))
                return r1 and r2
            except Exception as ex:
                logging.error(f'MC.read_module_response: cannot parse payload length: {ex}')
                return False

    async def read_module_response(self):
        """Reads first an unsigned int, which is interpreted as the length of the payload, which the MC reponds to the RPI

        Returns:
            (unsigned int, byte-array): (payload length, payload)
        """
        async with self._rpi_lock:
            #read unsigned int corresponding to the payload size
            payload_length = None
            try:
                payload_length = unpack('>I', self.spi.readbytes(calcsize('>I')))
            except Exception as ex:
                logging.error(f'MC.read_module_response: cannot parse payload length: {ex}')
            
            if payload_length is not None:
                bytes_left = payload_length
                output = []
                try:
                    while bytes_left>0:
                        batchsize = min(bytes_left, 4000)
                        spiinput = self.spi.readbytes(batchsize)
                        output += list(spiinput)
                        bytes_left -= batchsize
                except Exception as ex:
                    logging.error(f'MC.read_module_response: cannot read payload of length: {payload_length}: {ex}')

                return payload_length, output
            else:
                return None

    async def write(self, bytes):
        async with self._rpi_lock:
            try:
                self._spi.writebytes2(bytes)
            except Exception as ex:
                logging.error(f'MC: Cannot send bytes to rpi: {ex}. len-bytes: {len(bytes)}')