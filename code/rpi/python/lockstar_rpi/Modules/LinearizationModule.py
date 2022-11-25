from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings
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
        self.max_package_size = floor((MCDataPackage.MAX_NBR_BYTES-100)/4)
        self.number_of_ramp_packages = 0
        self.number_of_full_ramp_packages = 0
    

    # ==== START: client methods

    async def initialize(self, writer):
        pass

    """
    Signals the uC to output the specified ramp and return the
    measured system response. Then calculates the inverse and sends it back
    to the uc
    """

    async def new_linearization(self,ramp_start,ramp_end,ramp_length,ramp_speed,writer,respond=True):

        logging.info('Backend: new_linearization')

        self.ramp_start = ramp_start
        self.ramp_end = ramp_end
        self.ramp_length = ramp_length
        self.ramp_speed = ramp_speed
        self.ramp = np.linspace(ramp_start,ramp_end,ramp_length)
        self.mc_prescaler = 200
        self.mc_auto_reload = 900
        # calculate_mc_timer_parameter()

        if not await self.set_ramp_parameters(): # Method identifier: 11
            logging.error('Backend: Could not start new linearization!')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
            
        logging.debug('new_linearization: Waiting for gain measurement...')
        
        sleep(3)
        
        if not await MC.I().read_ack():
            logging.error('new_linearization: gain measurement timeout!')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        
        logging.debug('new_linearization: Gain measurement received!')

        if not await self.send_gain_measurement(): # 12
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
            
        self.monotone_envelope = self.calculate_monotone_envelope(self.measured_gain)
        self.calculate_inverted_pivots()
        
        
        if not await self.set_inverted_pivots(): # 13
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        br = BackendResponse([self.measured_gain.tolist(),self.inverted_pivots.tolist()])
        writer.write(br.to_bytes())
        await writer.drain()
        
        logging.debug('Backend: New linearization successful!')
        
        return True

    # ==== END: client methods


    # ==== START: helper methods
    
    async def set_ramp_parameters(self):
        logging.debug('Backend: set_ramp_parameters')
        
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',11)
        mc_data_package.push_to_buffer('float',self.ramp_start)
        mc_data_package.push_to_buffer('float',self.ramp_end)
        mc_data_package.push_to_buffer('uint32_t',self.ramp_length)
        mc_data_package.push_to_buffer('uint32_t',self.mc_auto_reload)
        mc_data_package.push_to_buffer('uint32_t',self.mc_prescaler)
        
        await MC.I().write_mc_data_package(mc_data_package)
        
        if not await MC.I().read_ack():
            logging.error('set_ramp_parameters: Could not set ramp parameters')
            return False
            
        logging.debug('new_linearization: Set ramp parameters')
        return True

    async def send_gain_measurement(self):
        METHOD_IDENTIFIER = 12
        logging.debug('Backend: send_gain_measurement')
        
        self.number_of_ramp_packages = ceil(self.ramp_length/self.max_package_size)
        self.number_of_full_ramp_packages = self.ramp_length//self.max_package_size
        
        print('max size ',self.max_package_size)
        print(self.number_of_ramp_packages)
        print(self.number_of_full_ramp_packages)
        
        gain_measurement_list = [0]*self.ramp_length
        buffer_offset = 0
        last_package = False
        for package_number in range(self.number_of_full_ramp_packages):
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
            mc_data_package.push_to_buffer('uint32_t',buffer_offset)
            mc_data_package.push_to_buffer('uint32_t',self.max_package_size)
            mc_data_package.push_to_buffer('bool',last_package)
            
            await MC.I().write_mc_data_package(mc_data_package)
            
            sleep(0.2)
            
            response_list = ['float']*self.max_package_size
            response_length,response_list = await MC.I().read_mc_data_package(response_list)
            start = package_number*self.max_package_size
            end = start + self.max_package_size
            gain_measurement_list[start:end] = response_list
            
            buffer_offset += self.max_package_size
            logging.debug('send_gain_measurement: received package number {package_number+1}')
            
        last_package = True
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
        mc_data_package.push_to_buffer('uint32_t',buffer_offset)
        mc_data_package.push_to_buffer('uint32_t',self.ramp_length%self.max_package_size)
        mc_data_package.push_to_buffer('bool',last_package)
        await MC.I().write_mc_data_package(mc_data_package)
        
        response_list = ['float']*(self.ramp_length%self.max_package_size)
        response_length,response_list = await MC.I().read_mc_data_package(response_list)
        gain_measurement_list[-(self.ramp_length%self.max_package_size):] = response_list
        
                
        self.measured_gain = np.array(gain_measurement_list)
        logging.debug('send_gain_measurement: Measurement received!')
        print(self.measured_gain)
        return True
        
        
    def calculate_monotone_envelope(self,data,increment=1e-6):
        envelope = [0]*len(data)
        old_value = data[0]
        for i,value in enumerate(data):
            envelope[i] = value
            if value<old_value:
                envelope[i] = old_value+increment
            old_value = envelope[i]
        return np.array(envelope)

    
    def calculate_inverted_pivots(self):
        logging.debug('Backend: calculate_inverted_pivots')
        self.pivots = np.linspace(min(self.monotone_envelope),max(self.monotone_envelope),len(self.ramp))
        inverted_gain = interpolate.interp1d(self.monotone_envelope,self.ramp)
        self.inverted_pivots = inverted_gain(self.pivots)
        return True
        
        

    async def set_inverted_pivots(self):
        METHOD_IDENTIFIER = 13
        logging.debug('Backend: set_inverted_pivots')
        
        buffer_offset = 0
        last_package = False
        
        print(self.inverted_pivots)
        
        for package_number in range(self.number_of_full_ramp_packages):                        
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',13)
            mc_data_package.push_to_buffer('uint32_t',buffer_offset)
            mc_data_package.push_to_buffer('uint32_t',self.max_package_size)
            mc_data_package.push_to_buffer('bool',last_package)
            for i in range(self.max_package_size):
                mc_data_package.push_to_buffer('float',self.inverted_pivots[buffer_offset + i])
            await MC.I().write_mc_data_package(mc_data_package)
            buffer_offset += self.max_package_size
            
            if not await MC.I().read_ack():
                logging.error('set_inverted_pivot_points: Could not set inverted pivot points')
                return False
            
        last_package = True        
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
        mc_data_package.push_to_buffer('uint32_t',buffer_offset)
        mc_data_package.push_to_buffer('uint32_t',self.ramp_length%self.max_package_size)
        mc_data_package.push_to_buffer('bool',last_package)
        for i in range(self.ramp_length%self.max_package_size):
            mc_data_package.push_to_buffer('float',self.inverted_pivots[buffer_offset + i])
        await MC.I().write_mc_data_package(mc_data_package)
        
        if not await MC.I().read_ack():
            logging.error('set_inverted_pivot_points: Could not set inverted pivot points')
            return False
        
        logging.debug('set_inverted_pivots: Success!')
        return True        
    
    def is_initialized(self):
        if self.ramp_length < 2:
            return False
        if self.ramp_speed <= 0:
            return False
        if self.ramp is None:
            return False
        if self.mc_prescaler < 0 or self.mc_prescaler > 0xFFFFFFFF:
            return False
        if self.mc_auto_reload < 0 or self.mc_auto_reload > 0xFFFFFFFF:
            return False
        return True

    # ==== END: helper methods
    
            

        
            
        
        

        


