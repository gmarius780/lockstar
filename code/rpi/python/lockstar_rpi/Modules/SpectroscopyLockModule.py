from lockstar_rpi.Modules.ScopeModule_ import ScopeModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class SpectroscopyLockModule(ScopeModule_):

    def __init__(self) -> None:
        super().__init__()
        self.p = 0
        self.i = 0
        self.d = 0
        self.dither_amp = 0
        self.dither_offset = 0
        self.locked = False

        self.dither_frq = 5

        #the scope is automatically setup by the MC in the SpectroscopyLockModule
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

    async def set_pid(self, p: float, i: float, d: float, writer, respond=True):
        logging.debug('SpectroscopyLockModule: set_pid')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.p = p
            self.i = i
            self.d = d
        return result

    async def lock(self, writer, respond=True):
        logging.debug('SpectroscopyLockModule: lock')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked= True
        return result
    
    async def unlock(self, writer, respond=True):
        logging.debug('SpectroscopyLockModule: unlock')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked = False
        return result

    async def set_dither(self, amp: float, offset: float, writer, respond=True):
        logging.debug('SpectroscopyLockModule: set_dither')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) # method_identifier
        mc_data_package.push_to_buffer('float', amp)
        mc_data_package.push_to_buffer('float', offset)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.dither_amp = amp
            self.dither_offset = offset
        return result

   
    async def set_dither_frq(self, dither_frq: float, writer, respond=True):
        """Set the dither (saw tooth waveform) frequency

        Args:
            dither_frq (float): Hz (not larger than 10Hz)
        """
        logging.debug('SpectroscopyLockModule: set_dither_frq')
        if dither_frq > 10:
            logging.error(f'SpectroscopyLockModule: dither frequency has to be smaller than 10Hz')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 15) # method_identifier
        mc_data_package.push_to_buffer('float', dither_frq)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            # set the appropritate scope sampling rate such that it resolves the new dither frequency
            max_scope_samples = max([self.scope_nbr_samples_in_one, self.scope_nbr_samples_in_two, self.scope_nbr_samples_out_one, self.scope_nbr_samples_out_two])
            await self.set_scope_sampling_rate(max_scope_samples*dither_frq, writer, respond=respond)

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
            
            #if the config dict contains the parameters of SpectroscopyLockModule, configure the MC
            if 'p' in config_dict.keys() and 'locked' in config_dict.keys():
                retry_counter = 10
                success = False
                while retry_counter > 0 and not success:
                    success = await self.set_pid(config_dict['p'], config_dict['i'], config_dict['d'], None)
                    success = success and await self.set_setpoint(config_dict['setpoint'], None)
                    success = success and await self.set_dither(config_dict['dither_amp'], config_dict['dither_offset'], None)
                    success = success and await self.set_dither_frq(config_dict['dither_frq'], None)

                    if config_dict['locked'] == True:
                        success = success and await self.lock(None)
                    retry_counter -= 1
            
        except Exception as ex:
            logging.error(f'SpectroscopyLockModule: canot launch_from_config: {ex}')
            


