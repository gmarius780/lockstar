from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.mc_modules import SinglePIDModuleDP
from lockstar_general.utils import int_to_HardwareComponents
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
import asyncio
from time import sleep


class SinglePIDModule(IOModule_):
    def __init__(self) -> None:
        super().__init__()
        self.p = None
        self.i = None
        self.d = None
        self.out_range_min = None
        self.out_range_max = None
        self.useTTL = None
        self.locked = None


    # ==== START: client methods 
    async def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, useTTL: bool, locked: bool, writer):
        self.p = p
        self.i = i
        self.d = d
        self.out_range_min = out_range_min
        self.out_range_max = out_range_max
        self.useTTL = useTTL
        self.locked = locked

        logging.debug('Starting initialization: SinglePIDModule')

        #=== sequentially send configuration to MC
        ack = self.set_pid(p, i, d, writer, respond=False)
        ack = ack and self.set_output_limits(out_range_min, out_range_max, writer, respond=False)

        if locked:
            ack = ack and self.lock(writer, respond=False)
        else:
            ack = ack and self.unlock(writer, respond=False)

        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    async def check_for_ack(self, writer=None):
        ack =  await MC.I().read_ack()
        if ack and writer is not None:
            writer.write(BackendResponse.ACK().to_bytes())
        else:
            writer.write(BackendResponse.NACK().to_bytes())
        if writer is not None:
            await writer.drain()
        
        return ack

    async def set_pid(self, p: float, i: float, d: float, writer, respond=True):
        self.p = p
        self.i = i
        self.d = d

        logging.debug('Backend: set_pid')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        await MC.I().write_mc_data_package(mc_data_package)
        
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_output_limits(self, min: float, max: float, writer, respond=True):
        self.out_range_min = min
        self.out_range_max = max

        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def lock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def unlock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
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
            await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
                                config_dict['out_range_max'], config_dict['useTTL'], config_dict['locking'], None)

            await super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'SinglePIDModule: canot launch_from_config: {ex}')
            raise ex


