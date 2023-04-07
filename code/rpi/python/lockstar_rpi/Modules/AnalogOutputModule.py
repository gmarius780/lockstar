from lockstar_rpi.Modules.ScopeModule_ import ScopeModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class AnalogOutputModule(ScopeModule_):
    def __init__(self) -> None:
        super().__init__()
        self.is_output_on = False
        self.is_output_ttl = False
        self.output_value_one = 0.
        self.output_value_two = 0.

    # ==== START: client methods 
    async def initialize(self, writer):
        pass

    async def check_for_ack(self, writer=None):
        ack =  await MC.I().read_ack()
        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    async def output_on(self, writer, respond=True):
        logging.debug('Backend: output_on')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))
    
    async def output_off(self, writer, respond=True):
        logging.debug('Backend: output_off')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def output_ttl(self, writer, respond=True):
        logging.debug('Backend: output_ttl')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))


    async def set_ch_one_output(self, value: float, writer, respond=True):
        self.output_value_one = value

        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 16) # method_identifier
        mc_data_package.push_to_buffer('float', value)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_two_output(self, value: float, writer, respond=True):
        self.output_value_two = value

        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 17) # method_identifier
        mc_data_package.push_to_buffer('float', value)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    
    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        for key in self.__dict__:
            if key not in config:
                config[key] = self.__dict__[key]
        return config

    async def launch_from_config(self, config_dict):
        try:
            # await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
            #                     config_dict['out_range_max'], config_dict['useTTL'], config_dict['locked'], None)

            await super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'SinglePIDModule: canot launch_from_config: {ex}')
            raise ex


