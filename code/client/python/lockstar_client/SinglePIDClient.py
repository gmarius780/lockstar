import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall

class SinglePIDClient(LockstarClient):
    """Basic Module which implements a simple PID controller by using input_1 as error signal, 
    input_2 as setpoint and output 1 for the control signal"""
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'SinglePIDModule')


    # @staticmethod
    # async def call(coroutine):
    #     try:
    #         loop = asyncio.get_running_loop()
    #     except RuntimeError:  # 'RuntimeError: There is no current event loop...'
    #         loop = None

    #     if loop is None:
    #         return asyncio.run(coroutine)
    #     else:
    #         return await coroutine

    async def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, locked: bool,
                    input_offset: float, output_offset: float, i_threshold: float, intensity_lock_mode: bool):
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
                            'locked': locked, 'input_offset': input_offset, 'output_offset': output_offset,
                            'i_threshold': i_threshold, 'intensity_lock_mode': intensity_lock_mode}
                        )
        
        # return asyncio.run(self._call_lockstar(bc))
        return await self._call_lockstar(bc)

    async def set_pid(self,  p: float, i: float, d: float, input_offset: float, output_offset: float, i_threshold: float):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'set_pid', args={'p': p, 'i': i, 'd': d, 
                    'input_offset': input_offset, 'output_offset': output_offset, 'i_threshold': i_threshold})
        return await self._call_lockstar(bc)

    async def set_output_limits(self,  min: float, max: float):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'set_output_limits', args={'min': min, 'max': max})
        return await self._call_lockstar(bc)

    async def lock(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'lock', args={})
        return await self._call_lockstar(bc)

    async def unlock(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'unlock', args={})
        return await self._call_lockstar(bc)

    async def enable_intensity_lock_mode(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'enable_intensity_lock_mode', args={})
        return await self._call_lockstar(bc)

    async def disable_intensity_lock_mode(self):
        bc = BackendCall(self.client_id, 'SinglePIDModule', 'disable_intensity_lock_mode', args={})
        return await self._call_lockstar(bc)

client = None
async def main():
    from os.path import join, dirname
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )
    client = SinglePIDClient('192.168.88.25', 10780, 1234)
    response = await client.initialize(1,0,0,0,10,False, 0, 0, 0, False)
    
    initialized = False

    if response.is_wrong_client_id():
        if await client.register_client_id():
            logging.info(f'Registered my client id: {client.client_id}')
            response = client.initialize(1,0,0,0,10,False, 0, 0, 0, False)

            initialized = response.is_ACK()
        else:
            logging.info(f'Failed to register my client id: {client.client_id}')

    else:
        initialized = True
    
    if initialized:
        logging.info(f'Successfully initialized Single PID module')
        # linearization_file = join(dirname(__file__), 'test_linearization.json')
        linearization_file = 'test_linearization.json'
        linearization_length = 2000
        print(client.set_ch_one_output_limits(0, 10))
        print(await client.set_linearization_length_one(linearization_length))
        print(await client.set_linearization_one_from_file(linearization_file))
        #print(await client.disable_linearization_one())

if __name__ == "__main__":
    asyncio.run(main())