from lockstar_rpi.Modules.BufferBaseModule_ import BufferBaseModule_
import logging

class AWGModule(BufferBaseModule_):
    BUFFER_LIMIT_kBYTES = 180
    MAX_NBR_OF_CHUNKS = 100


    def __init__(self) -> None:
        super().__init__()


    # ==== START: client methods 


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


