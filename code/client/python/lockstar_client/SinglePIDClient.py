import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall

class SinglePIDClient(LockstarClient):
    """Basic Module which implements a simple PID controller by using input_1 as setpoint, 
    input_2 as error_signal and output 1 for the control signal"""
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id)


    def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, locked: bool,
                    input_offset: float, output_offset: float):
        """Set all system module parameters

        Args:
        :param    p (float): p
        :param    i (float): i
        :param    d (float): d
        :param    out_range_min (float): output range minimum in volt
        :param    out_range_max (float): output range maximum in volt
        :param    locked (bool): lock
        :param    input_offset (float): voltage to be added to the error-signal (to compensate PD offsets)
        :param    output_offset (float): voltage to be added to the control signal --> e.g. to compensate for 'break-through' voltages in diodes
        """
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'initialize',
                        args={'p': p, 'i': i, 'd': d, 'out_range_min': out_range_min, 'out_range_max': out_range_max, 
                            'locked': locked, 'input_offset': input_offset, 'output_offset': output_offset}
                        )
        
        return asyncio.run(self._call_lockstar(bc))

    def set_pid(self,  p: float, i: float, d: float, input_offset: float, output_offset: float):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'set_pid', args={'p': p, 'i': i, 'd': d, 
                    'input_offset': input_offset, 'output_offset': output_offset})
        return asyncio.run(self._call_lockstar(bc))

    def set_output_limits(self,  min: float, max: float):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'set_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def lock(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'lock', args={})
        return asyncio.run(self._call_lockstar(bc))

    def unlock(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'unlock', args={})
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
    client = SinglePIDClient('192.168.88.201', 10780, 1234

    response = client.initialize(1,0,0,0,10,False, 0, 0)
    
    initialized = False

    if response.is_wrong_client_id():
        if client.register_client_id():
            logging.info(f'Registered my client id: {client.client_id}')
            response = client.initialize(1,0,0,0,5,False)

            initialized = response.is_ACK()
        else:
            logging.info(f'Failed to register my client id: {client.client_id}')

    else:
        initialized = True
    
    if initialized:
        logging.info(f'Successfully initialized Single PID module')

