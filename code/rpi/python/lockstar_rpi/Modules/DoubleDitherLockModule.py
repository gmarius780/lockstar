from lockstar_rpi.Modules.ScopeModule_ import ScopeModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class DoubleDitherLockModule(ScopeModule_):

    def __init__(self) -> None:
        super().__init__()
        self.p_one = 0
        self.i_one = 0
        self.d_one = 0
        self.setpoint_one = 0
        self.dither_amp_one = 0
        self.dither_offset_one = 0
        self.locked_one = False

        self.p_two = 0
        self.i_two = 0
        self.d_two = 0
        self.setpoint_two = 0
        self.dither_amp_two = 0
        self.dither_offset_two = 0
        self.locked_two = False

        self.dither_frq = 5

        #the scope is automatically setup by the MC in the DoubleDitherLockModule
        self.scope_max_buffer_length_nbr_of_floats = 2000
    


    # ==== START: client methods 
    async def check_for_ack(self, writer=None):
        ack =  await MC.I().read_ack()
        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    async def set_pid_one(self, p: float, i: float, d: float, writer, respond=True):
        logging.debug('DoubleDitherLockModule: set_pid_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.p_one = p
            self.i_one = i
            self.d_one = d
        return result
    
    async def set_pid_two(self, p: float, i: float, d: float, writer, respond=True):
        logging.debug('DoubleDitherLockModule: set_pid_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.p_two = p
            self.i_two = i
            self.d_two = d
        return result

    async def lock_one(self, writer, respond=True):
        logging.debug('DoubleDitherLockModule: lock_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked_one = True
        return result
    
    async def lock_two(self, writer, respond=True):
        logging.debug('DoubleDitherLockModule: lock_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked_two = True
        return result

    async def unlock_one(self, writer, respond=True):
        logging.debug('DoubleDitherLockModule: unlock_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 15) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked_one = False
        return result

    async def unlock_two(self, writer, respond=True):
        logging.debug('DoubleDitherLockModule: unlock_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 16) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked_two = False
        return result

    async def set_dither_one(self, amp: float, offset: float, writer, respond=True):
        logging.debug('DoubleDitherLockModule: set_dither_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 17) # method_identifier
        mc_data_package.push_to_buffer('float', amp)
        mc_data_package.push_to_buffer('float', offset)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.dither_amp_one = amp
            self.dither_offset_one = offset
        return result

    async def set_dither_two(self, amp: float, offset: float, writer, respond=True):
        logging.debug('DoubleDitherLockModule: set_dither_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 18) # method_identifier
        mc_data_package.push_to_buffer('float', amp)
        mc_data_package.push_to_buffer('float', offset)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.dither_amp_two = amp
            self.dither_offset_two = offset
        return result

    async def set_setpoint_one(self, setpoint: float, writer, respond=True):
        logging.debug('DoubleDitherLockModule: set_setpoint_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 19) # method_identifier
        mc_data_package.push_to_buffer('float', setpoint)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.setpoint_one = setpoint
        return result

    async def set_setpoint_two(self, setpoint: float, writer, respond=True):
        logging.debug('DoubleDitherLockModule: set_setpoint_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 20) # method_identifier
        mc_data_package.push_to_buffer('float', setpoint)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.setpoint_two = setpoint
        return result
    
    async def set_dither_frq(self, dither_frq: float, writer, respond=True):
        """Set the dither (saw tooth waveform) frequency

        Args:
            dither_frq (float): Hz (not larger than 10Hz)
        """
        logging.debug('DoubleDitherLockModule: set_dither_frq')
        if dither_frq > 10:
            logging.error(f'DoubleDitherLockModule: dither frequency has to be smaller than 10Hz')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 21) # method_identifier
        mc_data_package.push_to_buffer('float', dither_frq)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            # set the appropritate scope sampling rate such that it resolves the new dither frequency
            max_scope_samples = max([self.scope_nbr_samples_in_one, self.scope_nbr_samples_in_two, self.scope_nbr_samples_out_one, self.scope_nbr_samples_out_two])
            self.set_scope_sampling_rate(max_scope_samples*dither_frq, writer, respond=respond)

            self.dither_frq = dither_frq
        return result
    
    
    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        
        return config

    async def launch_from_config(self, config_dict):
        try:
            await super().launch_from_config(config_dict)
            
            #if the config dict contains the parameters of DoubleDitherLockModule, configure the MC
            if 'p_one' in config_dict.keys() and 'locked_one' in config_dict.keys() and 'p_two' in config_dict.keys():
                retry_counter = 10
                success = False
                while retry_counter > 0 and not success:
                    success = await self.set_pid_one(config_dict['p_one'], config_dict['i_one'], config_dict['d_one'], None)
                    success = success and await self.set_pid_two(config_dict['p_two'], config_dict['i_two'], config_dict['d_two'], None)
                    success = success and await self.set_setpoint_one(config_dict['setpoint_one'], None)
                    success = success and await self.set_setpoint_two(config_dict['setpoint_two'], None)
                    success = success and await self.set_dither_one(config_dict['dither_amp_one'], config_dict['dither_offset_one'], None)
                    success = success and await self.set_dither_two(config_dict['dither_amp_two'], config_dict['dither_offset_two'], None)
                    success = success and await self.set_dither_frq(config_dict['dither_frq'], None)

                    if config_dict['locked_one'] == True:
                        success = success and await self.lock_one(None)
                    if config_dict['locked_two'] == True:
                        success = success and await self.lock_two(None)
                    retry_counter -= 1
            
        except Exception as ex:
            logging.error(f'DoubleDitherLockModule: canot launch_from_config: {ex}')
            


