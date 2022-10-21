from lockstar_rpi.Modules.BufferBaseModule_ import BufferBaseModule_
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class AWGPIDModule(BufferBaseModule_):


    def __init__(self) -> None:
        super().__init__()
        self.p_one = None
        self.i_one = None
        self.d_one = None
        self.p_two = None
        self.i_two = None
        self.d_two = None

        self.locked = None


    # ==== START: client methods 

    async def set_pid_one(self, p: float, i: float, d: float, writer, respond=True):
        self.p_one = p
        self.i_one = i
        self.d_one = d

        logging.debug('Backend: set_pid')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 31) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        await MC.I().write_mc_data_package(mc_data_package)
        
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_pid_two(self, p: float, i: float, d: float, writer, respond=True):
        self.p_two = p
        self.i_two = i
        self.d_two = d

        logging.debug('Backend: set_pid')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 32) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        await MC.I().write_mc_data_package(mc_data_package)
        
        return await self.check_for_ack(writer=(writer if respond else None))

    async def lock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 33) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def unlock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 34) # method_identifier
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
            pass
            # await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
            #                     config_dict['out_range_max'], config_dict['useTTL'], config_dict['locked'], None)

            # await  super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'AWGModule: canot launch_from_config: {ex}')
            raise ex


