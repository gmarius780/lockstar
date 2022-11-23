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
    
        self.ramp = None
        self.pivots = None
        self.inverted_pivots = None
        self.ramp_length = 0
        self.ramp_speed = 0
        self.mc_prescaler = 0
        self.mc_auto_reload = 0
        self.measured_gain = None
        self.monotone_envelope = None
        self.ramp_package_sizes = None
    

    # ==== START: client methods

    async def initialize(self, writer):
        pass

    """
    Signals the uC to output the specified ramp and return the
    measured system response. Then calculates the inverse and sends it back
    to the uc
    """

    async def new_linearization(self,ramp,ramp_speed,writer,respond=True):

        logging.info('Backend: new_linearization')

        self.ramp = ramp
        self.ramp_length = len(ramp)
        self.ramp_speed = ramp_speed
        self.mc_prescaler = 200
        self.mc_auto_reload = 900
        # calculate_mc_timer_parameter()

        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',11)
        await MC.I().write_mc_data_package(mc_data_package)
        if not await MC.I().read_ack():
            logging.error('new_linearization: Could not start new linearization!')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        if not await self.initialize_timer(): # Method_identifier: 12
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        if not await self.initialize_new_ramp(): # 13
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        if not await self.set_ramp(): # 14
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        if not await self.trigger_gain_measurement(): # 15
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False

        logging.debug('new_linearization: Waiting for gain measurement result...')
        sleep(1.5)

        if not await self.send_gain_measurement(): # 16
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
            
        self.monotone_envelope = self.calculate_monotone_envelope(self.measured_gain)
        self.calculate_inverted_pivots()
        
        if not await self.set_inverted_pivots(): # 17
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

    async def initialize_timer(self):
        logging.debug('Backend: initialize_timer')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',12)
        mc_data_package.push_to_buffer('uint32_t',self.mc_auto_reload)
        mc_data_package.push_to_buffer('uint32_t',self.mc_prescaler)

        logging.debug('initialize_timer: Sending ARR,PSC...')
        await MC.I().write_mc_data_package(mc_data_package)
        if not await MC.I().read_ack():
            logging.error('send_ramp_to_mc: Could not set initialize timer')
            return False
        logging.debug('send_ramp_to_mc: Ramp parameters sent successfully')
        return True


    async def initialize_new_ramp(self): 
        logging.debug('Backend: initialize_new_ramp')    
        if(self.ramp_length < 2):
            logging.error('initialize_new_ramp: ramp_length too small (ramp_length={self.ramp_length})')
            return False

        # send the ramp_length and wait for ACK
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',13)
        mc_data_package.push_to_buffer('uint32_t',self.ramp_length)
        logging.debug('initialize_new_ramp: Sending ramp_length...')
        await MC.I().write_mc_data_package(mc_data_package)
        if not await MC.I().read_ack():
            logging.error('initialize_new_ramp: Could not set ramp length')
            return False
        logging.debug('initialize_new_ramp: Ramp length set successfully')
        return True

    async def set_ramp(self):        
        logging.debug('Backend: set_ramp')
        
        max_package_size = floor((MCDataPackage.MAX_NBR_BYTES-100)/4)
        number_of_ramp_packages = ceil(self.ramp_length/max_package_size)
        self.ramp_package_sizes = [0]*number_of_ramp_packages
        for i in range(self.ramp_length//max_package_size):
            self.ramp_package_sizes[i] = max_package_size
        if(self.ramp_length%max_package_size != 0):
            self.ramp_package_sizes[-1] = self.ramp_length%max_package_size
            
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',14)
        mc_data_package.push_to_buffer('uint32_t',number_of_ramp_packages)
        await MC.I().write_mc_data_package(mc_data_package)
        
        number_of_sent_ramp_values = 0        
        for package_number in range(number_of_ramp_packages):
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',14)
            mc_data_package.push_to_buffer('uint32_t',self.ramp_package_sizes[package_number])
            start = number_of_sent_ramp_values
            end = start+self.ramp_package_sizes[package_number]
            for value in self.ramp[start:end]:
                mc_data_package.push_to_buffer('float',value)
            logging.debug('initialize_new_ramp: Sending ramp array...')
            await MC.I().write_mc_data_package(mc_data_package)
            #sleep(1)
        
        #sleep(1)
        if not await MC.I().read_ack():
            logging.error('initialize_new_ramp: Could not set ramp')
            return False
        
        logging.debug('initialize_new_ramp: Set ramp successfully')
        return True       
        

    async def trigger_gain_measurement(self):
        logging.debug('Backend: trigger_gain_measurement')
        
        if not self.is_initialized():
            logging.error('trigger_gain_measuremen:: Ramp or timer not initialized properly... Aborting')
            return False

        # Trigger gain measurement
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 15)
        logging.debug('trigger_gain_measurement: Sending trigger signal...')
        await MC.I().write_mc_data_package(mc_data_package)
        #sleep(1)
        if(not await MC.I().read_ack()):
            logging.error('trigger_gain_measurement: Failed to trigger gain measurement!')
            return False
        logging.debug('trigger_gain_measurement: Sent trigger successfully!')
        return True        


    async def send_gain_measurement(self):
        logging.debug('Backend: send_gain_measurement')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',16)
        logging.debug('send_gain_measurement: requesting measurement...')
        await MC.I().write_mc_data_package(mc_data_package)
        #if not await MC.I().read_ack():
        #    logging.error('send_gain_measurement: Could not request gain measurement!')
        #    return False
        logging.debug('send_gain_measurement: sent request successfully!')
        
        max_package_size = floor((MCDataPackage.MAX_NBR_BYTES-100)/4)
        number_of_ramp_packages = ceil(self.ramp_length/max_package_size)
        response_list = ['float']*self.ramp_length
        number_of_received_values = 0
        for package_number in range(number_of_ramp_packages):
            start = number_of_received_values
            end = start+self.ramp_package_sizes[package_number]
            response_length,response_list[start:end] = await MC.I().read_mc_data_package(response_list)
            number_of_received_values += self.ramp_package_sizes[package_number]
            
        if(len(response_list) != self.ramp_length):
            logging.error('send_gain_measurement: Measured response_length not matching ramp_length!')
            return False
        self.measured_gain = np.array(response_list)
        logging.debug('send_gain_measurement: Measurement received!')
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
        max_package_size = floor((MCDataPackage.MAX_NBR_BYTES-100)/4)
        number_of_ramp_packages = ceil(self.ramp_length/max_package_size)
        ramp_package_sizes = [0]*number_of_ramp_packages
        for i in range(self.ramp_length//max_package_size):
            ramp_package_sizes[i] = max_package_size
        if(self.ramp_length%max_package_size != 0):
            ramp_package_sizes[-1] = self.ramp_length%max_package_size
        
        number_of_sent_ramp_values = 0        
        for package_number in range(number_of_ramp_packages):
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',17)
            start = number_of_sent_ramp_values
            end = start+ramp_package_sizes[package_number]
            for value in self.inverted_pivots[start:end]:
                mc_data_package.push_to_buffer('float',value)
            logging.debug('initialize_new_ramp: Sending inverted pivots array...')
            await MC.I().write_mc_data_package(mc_data_package)
            sleep(1)
        
        sleep(1)
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
    
            

        
            
        
        

        


