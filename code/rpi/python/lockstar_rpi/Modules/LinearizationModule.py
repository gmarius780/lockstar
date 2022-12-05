
from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings
# BackendSettings.debug_mode = True
from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
import numpy as np
from scipy import interpolate
from math import floor,ceil


class LinearizationModule(IOModule_):

    def __init__(self) -> None:
        super().__init__()
    
        self.ramp_start = 0
        self.ramp_end = 0
        self.ramp_length = 0
        self.ramp_speed = 0
        self.ramp = None
        self.monotone_envelope = None
        
        self.pivots = None
        self.inverted_pivots = None
        
        self.mc_prescaler = 0
        self.mc_auto_reload = 0
        self.measured_gain = None
        
        self.ramp_package_sizes = None
        self.number_of_ramp_packages = 0
        self.number_of_full_ramp_packages = 0
    

    # ==== START: client methods

    async def initialize(self, writer):
        pass

    async def linearize_ch_one(self, ramp_start:float, ramp_end:float, ramp_length:int, settling_time_ms:int,writer):
        return await self.linearize_ch(True, ramp_start, ramp_end, ramp_length, settling_time_ms,writer)

    async def linearize_ch_two(self, ramp_start:float, ramp_end:float, ramp_length:int, settling_time_ms:int,writer):
        return await self.linearize_ch(False, ramp_start, ramp_end, ramp_length, settling_time_ms,writer)

    async def linearize_ch(self, ch_one:bool, ramp_start:float, ramp_end:float, ramp_length:int, settling_time_ms:int,writer):
        """
        Allows the user to create a new set of inverted pivot points based on a static gain measurement.
        The ramp is transmitted to the uC as (start value, end value and number of points). 
        Calling new_linearization triggers the following procedure:
            - Ramp parameters are transferred to the uC
            - uC outputs the ramp on DAC1 and measures the response on ADC channel 1
            - uC sends measured gain to rpi
            - Inverted pivot points are calculated and sent back to uC (number of pivot points = ramp_length).
            Pivot points are equidistantly spaced over [ramp_start,ramp_end]
        Args:
        :param      ramp_start (float): start value of ramp
        :param      ramp_end (float):   end value of ramp
        :param      ramp_length (int):  number of points in ramp array
        :param      settling_time_ms (int):   time in ms to wait between applying voltage to system and measuring
        """

        logging.info('Backend: new_linearization')

        #=== set ramp parameters
        # if not await self.set_ramp_parameters(ramp_start, ramp_end, ramp_length, settling_time_ms):
        #     logging.error('set_ramp_parameters: Could not set ramp parameters!')
        #     writer.write(BackendResponse.NACK().to_bytes())
        #     await writer.drain()
        #     return False

        #=== start gain measurement
        mc_data_package = MCDataPackage()
        if ch_one:
            mc_data_package.push_to_buffer('uint32_t',12) # METHOD_IDENTIFIER OF linearize_ch_one
        else:
            mc_data_package.push_to_buffer('uint32_t',13) # METHOD_IDENTIFIER OF linearize_ch_two
        await MC.I().write_mc_data_package(mc_data_package)
        
        gain_measurement_timeout = max(10, ceil(ramp_length * settling_time_ms / 1000))
        logging.debug('new_linearization: Waiting for gain measurement...')
        sleep(10)
        if not await MC.I().read_ack(timeout_s=None):
            logging.error('linearize_ch: gain measurement took longer than timeout --> fail')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        logging.debug('new_linearization: gain measurement done, retrieving result..')

        #=== retrieve gain measurement result
        gain_measurement_result = await self.get_gain_measurement_result(ramp_length)
        if gain_measurement_result is None:
            logging.error('gain measurement could not get result')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        #=== calculate linearization from gain measurement
        monotone_envelope = LinearizationModule.calculate_monotone_envelope(gain_measurement_result)
        ramp = np.linspace(ramp_start, ramp_end, num=ramp_length)
        linearization_pivots = LinearizationModule.calculate_inverted_pivots(monotone_envelope, ramp)
        
        br = BackendResponse([gain_measurement_result.tolist(), linearization_pivots.tolist()])
        writer.write(br.to_bytes())
        await writer.drain()
        logging.debug('Backend: New linearization successful!')
        return True

    async def output_test_ramp_ch_one(writer):
        METHOD_IDENTIFIER = 15
        logging.debug('Backend: output_test_ramp_ch_one')

        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
        await MC.I().write_mc_data_package(mc_data_package)
        
        if not await MC.I().read_ack(timeout_s=1):
            logging.error('output_test_ramp_ch_one: failed')
            return False
        return True

    async def output_test_ramp_ch_two(writer):
        METHOD_IDENTIFIER = 16
        logging.debug('Backend: output_test_ramp_ch_two')

        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
        await MC.I().write_mc_data_package(mc_data_package)
        
        if not await MC.I().read_ack(timeout_s=1):
            logging.error('output_test_ramp_ch_two: failed')
            return False
        return True
    # ==== END: client methods


    # ==== START: MC Methods
    
    async def set_ramp_parameters(self, ramp_start:float, ramp_end:float, ramp_length:int, settling_time_ms:int, writer):
        """Sends ramp parameters (ramp_start, ramp_end, ramp_length, ramp_speed) to uC"""
        METHOD_IDENTIFIER = 11
        logging.debug('Backend: set_ramp_parameters')

        if ramp_length <2 or settling_time_ms <=0:
            logging.error('set_ramp_parameters: Ramp parameters invalid!')
            return False
        
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
        mc_data_package.push_to_buffer('float',ramp_start)
        mc_data_package.push_to_buffer('float',ramp_end)
        mc_data_package.push_to_buffer('uint32_t',ramp_length)
        mc_data_package.push_to_buffer('uint32_t',settling_time_ms)
        await MC.I().write_mc_data_package(mc_data_package)
        sleep(5)
        if not await MC.I().read_ack():
            logging.error('linearize_ch: set_ramp_parameters failed')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            writer.write(BackendResponse.ACK().to_bytes())
            await writer.drain()
            return True


    async def get_gain_measurement_result(self, ramp_length:int):
        """Requests the gain measurement from uC"""
        METHOD_IDENTIFIER = 14
        logging.debug('Backend: get_gain_measurement_result')
        
        max_package_size = floor((MCDataPackage.MAX_NBR_BYTES-100)/4)
        number_of_ramp_packages = ceil(ramp_length/max_package_size)
        number_of_full_ramp_packages = ramp_length//max_package_size
                
        gain_measurement_list = [0]*ramp_length
        buffer_offset = 0
        logging.debug(f"get_gain_measurement_result: Expecting {number_of_ramp_packages} packages.")
        for package_number in range(number_of_full_ramp_packages):
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
            mc_data_package.push_to_buffer('uint32_t',buffer_offset)
            mc_data_package.push_to_buffer('uint32_t',max_package_size)
            await MC.I().write_mc_data_package(mc_data_package)
            sleep(0.2)
            
            response_list = ['float']*max_package_size
            response_length, response_list = await MC.I().read_mc_data_package(response_list)
            start = package_number*max_package_size
            end = start + max_package_size
            gain_measurement_list[start:end] = response_list
            
            buffer_offset += max_package_size
            logging.debug(f"get_gain_measurement_result: received package number {package_number+1}.")
            
        if number_of_full_ramp_packages < number_of_ramp_packages:
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
            mc_data_package.push_to_buffer('uint32_t',buffer_offset)
            mc_data_package.push_to_buffer('uint32_t',ramp_length%max_package_size)
            await MC.I().write_mc_data_package(mc_data_package)
        
            response_list = ['float']*(ramp_length%max_package_size)
            response_length,response_list = await MC.I().read_mc_data_package(response_list)
            gain_measurement_list[-(ramp_length%max_package_size):] = response_list       
        
        measured_gain = np.array(gain_measurement_list)
        logging.debug('get_gain_measurement_result: Measurement received!')
        return measured_gain
        
    @staticmethod  
    def calculate_monotone_envelope(data,increment=1e-6):
        """ 
        Calculates a monotone envelope of the measured gain to insure it is invertible. 
        increment specifies by how much each successive data point should increase if they 
        happen to be equal.
        Args:
        :param      data (numpy.ndarray, shape=(N), dtype=float): measured gain
        :param      increment (float): if data[i]==data[i+1], then data[i+1] = data[i]+increment 
        """
        envelope = [0]*len(data)
        old_value = data[0]
        for i,value in enumerate(data):
            envelope[i] = value
            if value<old_value:
                envelope[i] = old_value+increment
            old_value = envelope[i]
        return np.array(envelope)

    
    @staticmethod
    def calculate_inverted_pivots(monotone_envelope, ramp):
        """Creates equally spaced pivots array and calculates gain^(-1)(pivots)"""
        pivots = np.linspace(min(monotone_envelope),max(monotone_envelope),len(ramp))
        inverted_gain = interpolate.interp1d(monotone_envelope, ramp)
        return inverted_gain(pivots)
        

    # ==== END: MC methods

if __name__ == "__main__":
    test = LinearizationModule()
    
            

        
            
        
        

        


