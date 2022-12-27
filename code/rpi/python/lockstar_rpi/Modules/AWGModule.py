from lockstar_rpi.Modules.BufferBaseModule_ import BufferBaseModule_
import logging

class AWGModule(BufferBaseModule_):
    """ Allows user to upload arbitrary waveforms into two buffers (corresponding to the two analog outputs), splitting
    these waveforms into 'chunks' and outputting those chunks either via digital trigger or by calling output_on()
    """

    def __init__(self) -> None:
        super().__init__()


    # ==== START: client methods 


    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
        return config

    async def launch_from_config(self, config_dict):
        try:
            await  super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'AWGModule: canot launch_from_config: {ex}')


