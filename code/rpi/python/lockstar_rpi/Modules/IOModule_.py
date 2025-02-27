import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

from lockstar_rpi.Modules.Module import Module
from lockstar_general.backend.BackendResponse import BackendResponse
from math import floor, ceil
from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings


class IOModule_(Module):
    """This Module should not be used directly by the client (hence the _ at the end of the name).
    It contains functionality which is shared by most modules (e.g. linearization of the DAC)
    """
 

    def __init__(self) -> None:
        super().__init__()

        self.ramp_length_one = 0
        self.ramp_length_two = 0
        self.out_range_ch_one_min = 0
        self.out_range_ch_two_min = 0
        self.out_range_ch_one_max = 0
        self.out_range_ch_two_max = 0
        self.ch_one_linearization = []
        self.ch_two_linearization = []
        self.linearization_one_enabled = False
        self.linearization_two_enabled = False

    async def set_ch_one_output_limits(self, min: float, max: float, writer, respond=True):
        """Sets min and max output voltage in volt for channel one

        
        :param    min (float): userdefined: minimum output in volt
        :param    max (float): userdefined: maximum output in volt
        :param    writer (_type_): writer instance to respond to the client
        :param    respond (bool, optional): Defaults to True only False if this method is called
                    by launch_from_config.
        """
        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 80) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.out_range_ch_one_min = min
            self.out_range_ch_one_max = max
        return result

    async def set_ch_two_output_limits(self, min: float, max: float, writer, respond=True):
        """Sets min and max output voltage in volt for channel two

        
        :param    min (float): userdefined: minimum output in volt
        :param    max (float): userdefined: maximum output in volt
        :param    writer (_type_): writer instance to respond to the client
        :param    respond (bool, optional): Defaults to True only False if this method is called
                    by launch_from_config.
        """
        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 81) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.out_range_ch_two_min = min
            self.out_range_ch_two_max = max
        return result
    
    async def unclamp_output(self, writer, respond=True):
        logging.debug('Backend: unlcamp output')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 18) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))        

    #==== Linearization Methods START====
    async def set_linearization_one(self, linearization, min_output_voltage, max_output_voltage, writer, respond=True):
        """Sets the parameters used by the DAC_ONE to linearize the systems response"""
        return await self.set_linearization(linearization, min_output_voltage, max_output_voltage, True, writer, respond=respond)
    
    async def set_linearization_two(self, linearization, min_output_voltage, max_output_voltage, writer, respond=True):
        """Sets the parameters used by the DAC_ONE to linearize the systems response"""
        return await self.set_linearization(linearization, min_output_voltage, max_output_voltage, False, writer, respond=respond)
    
    async def set_linearization(self, linearization, min_output_voltage, max_output_voltage, ch_one, writer, respond=True):
        """Sends inverted pivot points to uC. End of linearization procedure"""
        if ch_one:
            METHOD_IDENTIFIER = 82
            ramp_length = self.ramp_length_one
        else:
            METHOD_IDENTIFIER = 83
            ramp_length = self.ramp_length_two
        logging.debug('Backend: set_linearization')

        if ramp_length != len(linearization):
            logging.error('set_linearization: linearization_length must match the set linearization length')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False

        max_package_size = floor((MCDataPackage.MAX_NBR_BYTES_TO_MC-100)/4)
        number_of_ramp_packages = ceil(ramp_length/max_package_size)
        number_of_full_ramp_packages = ramp_length//max_package_size

        buffer_offset = 0     
        for package_number in range(number_of_full_ramp_packages):                        
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
            mc_data_package.push_to_buffer('float',min_output_voltage)
            mc_data_package.push_to_buffer('float',max_output_voltage)
            if package_number > 0: 
                append = True
            else:
                append = False

            mc_data_package.push_to_buffer('bool',append)
            mc_data_package.push_to_buffer('uint32_t',max_package_size)

            for i in range(max_package_size):
                mc_data_package.push_to_buffer('float',linearization[buffer_offset + i])
            await MC.I().write_mc_data_package(mc_data_package)
            buffer_offset += max_package_size
            
            if not await MC.I().read_ack():
                logging.error('set_linearization: Could not set inverted pivot points')
                if writer is not None:
                    writer.write(BackendResponse.NACK().to_bytes())
                    await writer.drain()
                return False

        if number_of_ramp_packages > number_of_full_ramp_packages:     
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
            mc_data_package.push_to_buffer('float',min_output_voltage)
            mc_data_package.push_to_buffer('float',max_output_voltage)
            if number_of_full_ramp_packages > 0: 
                append = True
            else:
                append = False

            mc_data_package.push_to_buffer('bool',append)
            mc_data_package.push_to_buffer('uint32_t',ramp_length%max_package_size)
            
            
            for i in range(ramp_length%max_package_size):
                mc_data_package.push_to_buffer('float',linearization[buffer_offset + i])
            await MC.I().write_mc_data_package(mc_data_package)
        
            if not await MC.I().read_ack():
                logging.error('set_linearization: Could not set inverted pivot points')
                if writer is not None:
                    writer.write(BackendResponse.NACK().to_bytes())
                    await writer.drain()
                return False
        
        logging.debug('set_linearization: Success!')

        if ch_one:
            self.ch_one_linearization = linearization
            self.linearization_one_enabled = True
            self.out_range_ch_one_min = min_output_voltage
            self.out_range_ch_one_max = max_output_voltage
        else:
            self.ch_two_linearization = linearization
            self.linearization_two_enabled = True
            self.out_range_ch_two_min = min_output_voltage
            self.out_range_ch_two_max = max_output_voltage

        if writer is not None:
            writer.write(BackendResponse.ACK().to_bytes())
            await writer.drain()
        return True        

    async def set_linearization_length_one(self, linearization_length: int, writer, respond=True):
        logging.debug('Backend: set_linearization_length_one')
        if linearization_length > BackendSettings.MAX_LINEARIZATION_LENGTH:
            logging.error(f'linearization length must not be larger than {BackendSettings.MAX_LINEARIZATION_LENGTH} ({linearization_length})')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 84) # method_identifier
        mc_data_package.push_to_buffer('uint32_t', linearization_length)
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.ramp_length_one = int(linearization_length)
        return result

    async def set_linearization_length_two(self, linearization_length: int, writer, respond=True):
        logging.debug('Backend: set_linearization_length_two')
        if linearization_length > BackendSettings.MAX_LINEARIZATION_LENGTH:
            logging.error(f'linearization length must not be larger than {BackendSettings.MAX_LINEARIZATION_LENGTH} ({linearization_length})')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 85) # method_identifier
        mc_data_package.push_to_buffer('uint32_t', linearization_length)
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.ramp_length_two = int(linearization_length)
        return result

    async def enable_linearization_one(self, writer, respond=True):
        logging.debug('Backend: enable_linearization_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 86) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.linearization_one_enabled = True
        return result
    
    async def enable_linearization_two(self, writer, respond=True):
        logging.debug('Backend: enable_linearization_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 87) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.linearization_two_enabled = True
        return result

    async def disable_linearization_one(self, writer, respond=True):
        logging.debug('Backend: disable_linearization_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 88) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.linearization_one_enabled = False
        return result

    async def disable_linearization_two(self, writer, respond=True):
        logging.debug('Backend: disable_linearization_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 89) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.linearization_two_enabled = False
        return result

    async def check_for_ack(self, writer=None):
        """Waits for ACK/NACK from the MC and responds accordingly to the client"""
        sleep(0.05)
        ack =  await MC.I().read_ack()
        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    #==== Linearization Methods END====

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
        return config

    async def _set_linearization_one_from_config(self, config_dict):
        try:
            success = True
            success = await self.set_linearization_length_one(int(config_dict['ramp_length_one']), None, respond=False)
            if success:
                success = await self.set_linearization_one(config_dict['ch_one_linearization'], 
                                                    float(config_dict['out_range_ch_one_min']), 
                                                    float(config_dict['out_range_ch_one_max']), None, respond=False)
            
            if success and 'linearization_one_enabled' in config_dict.keys():
                success = await self.enable_linearization_one(None, respond=False)
        except Exception as ex_one:
            logging.error(f'IOModule: could not set linearization_one from config: {ex_one}')
            return False
        return success

    async def _set_linearization_two_from_config(self, config_dict):
        try:
            success = True
            success = await self.set_linearization_length_two(int(config_dict['ramp_length_two']), None, respond=False)
            if success:
                success = await self.set_linearization_two(config_dict['ch_two_linearization'], 
                                                    float(config_dict['out_range_ch_two_min']), 
                                                    float(config_dict['out_range_ch_two_max']), None, respond=False)
            
            if success and 'linearization_two_enabled' in config_dict.keys():
                success = await self.enable_linearization_two(None, respond=False)
        except Exception as ex:
            logging.error(f'IOModule: could not set linearization_two from config: {ex}')
            return False
        return success

    async def launch_from_config(self, config_dict):
        try:
            await super().launch_from_config(config_dict)
            
            if 'out_range_ch_one_min' in config_dict.keys() and 'out_range_ch_one_max' in config_dict.keys():
                await self.set_ch_one_output_limits(float(config_dict['out_range_ch_one_min']), float(config_dict['out_range_ch_one_max']), None, respond=False)
                #set linearization one
                if 'ramp_length_one' in config_dict.keys() and 'ch_one_linearization' in config_dict.keys() \
                        and config_dict['ch_one_linearization'] is not None and len(config_dict['ch_one_linearization']) == int(config_dict['ramp_length_one']) and \
                            int(config_dict['ramp_length_one'])>0:
                        retry_counter = 10
                        while retry_counter > 0 and not await self._set_linearization_one_from_config(config_dict): #retry if failed
                            retry_counter -= 1

            if 'out_range_ch_two_min' in config_dict.keys() and 'out_range_ch_two_max' in config_dict.keys():
                await self.set_ch_two_output_limits(float(config_dict['out_range_ch_two_min']), float(config_dict['out_range_ch_two_max']), None, respond=False)
                #set linearization two      
                if 'ramp_length_two' in config_dict.keys() and 'ch_two_linearization' in config_dict.keys() \
                        and config_dict['ch_two_linearization'] is not None and len(config_dict['ch_two_linearization']) == int(config_dict['ramp_length_two']) and \
                        int(config_dict['ramp_length_two']) > 0:
                    retry_counter = 10
                    while retry_counter > 0 and not await self._set_linearization_two_from_config(config_dict): #retry if failed
                        retry_counter -= 1     

        except Exception as ex:
            logging.error(f'IOModule: canot launch_from_config: {ex}')
