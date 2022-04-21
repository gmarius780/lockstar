import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.hardware import HardwareComponents
from lockstar_general.backend.BackendCall import BackendCall

class SinglePIDClient(LockstarClient):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id)


    def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, useTTL: bool, locked: bool,
                err_channel: HardwareComponents, setpoint_channel: HardwareComponents, out_channel: HardwareComponents):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'initialize',
                        args={'p': p, 'i': i, 'd': d, 'out_range_min': out_range_min, 
                                'out_range_max': out_range_max, 'useTTL': useTTL, 
                                'locked': locked, 'err_channel': err_channel.value, 
                                'setpoint_channel': setpoint_channel.value, 'out_channel': out_channel.value}
                        )
        
        return asyncio.run(self._call_lockstar(bc))

    def set_pid(self,  p: float, i: float, d: float):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'set_pid', args={'p': p, 'i': i, 'd': d})
        return asyncio.run(self._call_lockstar(bc))

    def lock(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'lock', args={})
        return asyncio.run(self._call_lockstar(bc))

    def unlock(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'unlock', args={})
        return asyncio.run(self._call_lockstar(bc))

    def enable_live_channel(self, ch: HardwareComponents):
        """Enables buffering on the MC of the ADC data for channel ch such that it can be polled from
        time to time with get_channel_data

        Args:
            ch (HardwareComponents): channel for which it should be enabled
        """
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'enable_live_channel', args={'ch': ch.value})
        return asyncio.run(self._call_lockstar(bc))

    def disable_live_channel(self, ch: HardwareComponents):
        """disable buffering on the MC of the ADC data for channel ch such that it can be polled from
        time to time with get_channel_data

        Args:
            ch (HardwareComponents): channel for which it should be enabled
        """
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'disable_live_channel', args={'ch': ch.value})
        return asyncio.run(self._call_lockstar(bc))

    def get_channel_data(self, ch: HardwareComponents):
        """read recend ADC data of a channel for which live mode is enabled

        Args:
            ch (HardwareComponents): channel from which data should be read
        """
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'get_channel_data', args={'ch': ch.value})
        return asyncio.run(self._call_lockstar(bc))

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )
    client = SinglePIDClient('192.168.88.13', 10780, 1234)

    response = client.initialize(1,2,3,4,5,True, False, 
                                HardwareComponents.analog_in_one, 
                                HardwareComponents.analog_in_two, 
                                HardwareComponents.analog_out_two)
    
    initialized = False

    if response.is_wrong_client_id():
        if client.register_client_id():
            logging.info(f'Registered my client id: {client.client_id}')
            response = client.initialize(1,2,3,4,5,True, False, 
                                HardwareComponents.analog_in_one, 
                                HardwareComponents.analog_in_two, 
                                HardwareComponents.analog_out_two)

            initialized = response.is_ACK()
        else:
            logging.info(f'Failed to register my client id: {client.client_id}')

    else:
        initialized = True
    
    if initialized:
        logging.info(f'Successfully initialized Single PID module')

