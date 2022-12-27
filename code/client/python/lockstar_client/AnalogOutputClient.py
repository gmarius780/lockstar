import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall

class AnalogOutputClient(LockstarClient):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'AnalogOutputModule')


    def initialize(self):
        pass
    
    def output_on(self):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'output_on', args={})
        return asyncio.run(self._call_lockstar(bc))

    def output_off(self):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'output_off', args={})
        return asyncio.run(self._call_lockstar(bc))

    def output_ttl(self):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'output_ttl', args={})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'set_ch_one_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_two_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'set_ch_two_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_output(self, value: float):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'set_ch_one_output', args={'value': value})
        return asyncio.run(self._call_lockstar(bc))
    
    def set_ch_two_output(self, value: float):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'set_ch_two_output', args={'value': value})
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
    client = AnalogOutputClient('192.168.88.201', 10780, 1234)
    client.register_client_id()
    client.set_ch_one_output_limits(0,1)
    # response = client.initialize(1,2,3,4,5,True, False)
    
    # initialized = False

    # if response.is_wrong_client_id():
    #     if client.register_client_id():
    #         logging.info(f'Registered my client id: {client.client_id}')
    #         response = client.initialize(1,2,3,4,5,True, False)

    #         initialized = response.is_ACK()
    #     else:
    #         logging.info(f'Failed to register my client id: {client.client_id}')

    # else:
    #     initialized = True
    
    # if initialized:
    #     logging.info(f'Successfully initialized Single PID module')

