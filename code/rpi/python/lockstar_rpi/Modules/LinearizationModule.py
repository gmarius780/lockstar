from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
import numpy as np


class LinearizationModule(IOModule_):



    def __init__(self) -> None:
        super().__init__()
    
        self.ramp = None
        self.linearized_ramp = None
        self.ramp_length = 0
        self.ramp_speed = 0
        self.mc_prescaler = 0
        self.mc_auto_reload = 0
        self.measured_response = None
        self.linearizer_parameters = []
    

    # ==== START: client methods

    async def initialize(self, writer):
        pass

    """
    Signals the uC to output the specified ramp and return the
    measured system response. Then calculates the inverse and sends it back
    to the uc
    """
    async def measure_response(self, writer, respond=True):
        
        logging.debug('Backend: measure_response')
        
        if not self.is_initialized():
            logging.error('measure_response: Ramp not initialized properly... Aborting')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        
        await self.send_ramp_to_mc()
        await self.initialize_mc_timer()
        
        # Start response measurement
        await self.start_response_measurement(writer,True)
        
        logging.debug('measure_response: Waiting for response measurement result')
        sleep(4)
        
        logging.debug('measure_response: Requesting measurement')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',15)
        await MC.I().write_mc_data_package(mc_data_package)
                
        response_list = ['float']*self.ramp_length
        response_length,response_list = await MC.I().read_mc_data_package(response_list)
        if(len(response_list) != self.ramp_length):
            logging.error('measure_response: Measured response_length not matching ramp_length')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        
        logging.debug('measure_response: Measurement received')

        self.measured_response = np.array(response_list)
        
        self.calculate_linearized_ramp()
        
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',17)
        for i in self.linearized_ramp:
            mc_data_package.push_to_buffer('float',i)
        await MC.I().write_mc_data_package(mc_data_package)
        
        if(not await MC.I().read_ack()):
            logging.error('measure_response: Could not send linearized ramp')
        
        br = BackendResponse([response_list,self.linearized_ramp.tolist()])
        #br = BackendResponse(self.linearized_ramp.tolist())
        writer.write(br.to_bytes())
        await writer.drain()
        
        logging.debug('Done!')
        
        return True
    
    def calculate_linearized_ramp(self):
        self.linearized_ramp = np.zeros_like(self.ramp)
        number_of_points = len(self.ramp)
        response_range = max(self.measured_response) - min(self.measured_response)
        ramp_range = max(self.ramp) - min(self.ramp)
        
        m = response_range/ramp_range
        b= min(self.measured_response)-m*min(self.ramp)
        
        ideal_linear_gain = lambda x: m*x+b
        
        # k_n_list and d_n_list contain the slopes and y-intersections of the piecewise linear approximation of measured_response
        k_n_list = np.zeros(len(self.ramp)-1)
        for i in range(len(k_n_list)):
            k_n_list[i] = (self.measured_response[i+1]-self.measured_response[i])/(self.ramp[i+1]-self.ramp[i])
        d_n_list = self.measured_response[:-1] - k_n_list*self.ramp[:-1]
        
        
        for i in range(len(self.linearized_ramp)):
            index = self.find_bin_index(ideal_linear_gain(self.ramp[i]))
            if(index is None):
                logging.error('calculate_linearized_ramp: An error occured during linearization!')
                return False
            if(index==number_of_points-1): index = number_of_points -2
            
            self.linearized_ramp[i] = (ideal_linear_gain(self.ramp[i])-d_n_list[index])/k_n_list[index]
            
    def find_bin_index(self,input):
        for i in range(len(self.measured_response)-1):
            if(self.measured_response[i] <= input and input < self.measured_response[i+1]):
                return i
            if(self.measured_response[-1] == input):
                return len(self.measured_response)-1
        return None
   
    async def start_response_measurement(self,writer,respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) ## S: why all start with 11?
        logging.debug('\n\n\n measure_response: Trying to start response measurement\n\n\n')
        
        await MC.I().write_mc_data_package(mc_data_package)
        if(not await MC.I().read_ack()):
            logging.error('measure_response: Failed to start response measurement')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        logging.debug('measure_response: Sarted response measurement successfully')
        
        
    
    async def initialize_mc_timer(self):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',13)
        mc_data_package.push_to_buffer('uint32_t',self.mc_auto_reload)
        mc_data_package.push_to_buffer('uint32_t',self.mc_prescaler)
        
        logging.debug('initialize_mc_timer: Trying to send PSC and ARR')
        await MC.I().write_mc_data_package(mc_data_package)
        ack = await MC.I().read_ack()
        if not ack:
            logging.error('initialize_mc_timer: Could not set PSC and ARR values')
            return False
        
        logging.debug('initialize_mc_timer: Timer initialized succesfully')
        return True
    
    async def send_ramp_to_mc(self):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',11)
        mc_data_package.push_to_buffer('uint32_t',self.ramp_length)
        logging.debug(f'send_ramp_to_mc: Trying to send ramp length {self.ramp_length}')
        # send the ramp_length and wait a bit
        await MC.I().write_mc_data_package(mc_data_package)
        
        ack = await MC.I().read_ack()
        if not ack:
            logging.error('send_ramp_to_mc: Could not set ramp length')
            return False
        logging.debug('send_ramp_to_mc: Ramp length sent successfully')
        sleep(1)
        
        # Send the ramp
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t',12)
        for i in self.ramp:
            mc_data_package.push_to_buffer('float',i)
        logging.debug('send_ramp_to_mc: Trying to send ramp')
        await MC.I().write_mc_data_package(mc_data_package)
        
        ack = await MC.I().read_ack()
        if not ack:
            logging.error('send_ramp_to_mc: Could not send ramp')
            return False
        
        logging.debug('send_ramp_to_mc: Set ramp successfully')
        return True
    
    async def set_ramp_speed(self,speed,writer,respond=True):
        if(speed < 0):
            logging.error('set_ramp_speed: invalid ramp_speed')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        
        self.ramp_speed = speed
        logging.debug(f'set_ramp_speed: Ramp speed set to {self.ramp_speed}')
        self.mc_prescaler = 50
        self.mc_auto_reload = 9000
        logging.debug('set_ramp_speed: Calculated prescaler = {,self.mc_prescaler}, ARR = {self.mc_auto_reload}')
        if(respond):
            writer.write(BackendResponse.ACK().to_bytes())
            await writer.drain()
        return True
    
    async def set_ramp_array(self,ramp,writer,respond=True):
        self.ramp_length = len(ramp)
        self.ramp = np.array(ramp)
        logging.debug('set_ramp_array: Ramp array initialized')
        print(self.ramp)
        if(respond):
            writer.write(BackendResponse.ACK().to_bytes())
            await writer.drain()
        return True        
        
    def check_for_ack(self,ack,log):
        if(ack):
            logging.debug(log)
        else:
            logging.error('MC NACK')
        return ack
    
    def is_initialized(self):
        if self.ramp_length > 0:
            if self.ramp_speed > 0:
                if self.ramp is not None:
                    return True
        return False
    
            

        
            
        
        

        


