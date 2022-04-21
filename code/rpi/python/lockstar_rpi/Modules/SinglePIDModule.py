from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.hardware import HardwareComponents
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.mc_modules import SinglePIDModuleDP
import logging
from lockstar_rpi.MC import MC


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
        self.err_channel = None
        self.setpoint_channel = None
        self.out_channel = None

    # ==== START: client methods 
    async def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, useTTL: bool, locked: bool,
                err_channel: HardwareComponents, setpoint_channel: HardwareComponents, out_channel: HardwareComponents, writer):
        self.p = p
        self.i = i
        self.d = d
        self.out_range_min = out_range_min
        self.out_range_max = out_range_max
        self.useTTL = useTTL
        self.locked = locked
        self.err_channel = err_channel if isinstance(err_channel, HardwareComponents) else HardwareComponents.from_int(err_channel)
        self.setpoint_channel = setpoint_channel if isinstance(setpoint_channel, HardwareComponents) else HardwareComponents.from_int(setpoint_channel)
        self.out_channel = out_channel if isinstance(out_channel, HardwareComponents) else HardwareComponents.from_int(out_channel)

        # Send to MC and await answer

        logging.debug('Initialized SinglePIDModule')

        # === MC CALL:
        #float p, float i, float d, float out_range_min, float out_range_max, bool useTTL,
		#	bool locked, HardwareComponents err_channel, HardwareComponents setpoint_channel, HardwareComponents out_channel
        write_bytes = SinglePIDModuleDP.write_initialize_call(p, i, d, out_range_min, out_range_max, useTTL, locked, err_channel, setpoint_channel, out_channel)
        MC.I().write(write_bytes)

        ack = await MC.I().read_ack()

        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    async def set_pid(self, p: float, i: float, d: float, writer):
        self.p = p
        self.i = i
        self.d = d

        # Send to MC and await answer

        logging.debug('Initialized set_pid')

        writer.write(BackendResponse.ACK())
        await writer.drain()

    async def lock(self):
        self.locked = True

    async def unlock(self):
        self.locked = False

    async def get_channel_data(self, ch: HardwareComponents):
        """read recend ADC data of a channel for which live mode is enabled

        Args:
            ch (HardwareComponents): channel from which data should be read
        """
        pass

    async def enable_live_channel(self, ch: HardwareComponents):
        """Enables buffering on the MC of the ADC data for channel ch such that it can be polled from
        time to time with get_channel_data

        Args:
            ch (HardwareComponents): channel for which it should be enabled
        """
        pass

    async def disable_live_channel(self, ch: HardwareComponents):
        """disable buffering on the MC of the ADC data for channel ch such that it can be polled from
        time to time with get_channel_data

        Args:
            ch (HardwareComponents): channel for which it should be enabled
        """
        pass
    
    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        for key in self.__dict__:
            if key not in config:
                if isinstance(self.__dict__[key], HardwareComponents):
                    config[key] = self.__dict__[key].value
                else:
                    config[key] = self.__dict__[key]
        return config

    async def launch_from_config(self, config_dict):
        try:
            await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
                                config_dict['out_range_max'], config_dict['useTTL'], config_dict['locking'], 
                                HardwareComponents.from_int(config_dict['err_channel']),
                                HardwareComponents.from_int(config_dict['setpoint_channel']),
                                HardwareComponents.from_int(config_dict['out_channel']), None)

            await super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'SinglePIDModule: canot launch_from_config: {ex}')
            raise ex


