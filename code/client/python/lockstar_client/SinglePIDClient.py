import asyncio
import logging
from lockstar_client.ScopeClient import ScopeClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall

class SinglePIDClient(ScopeClient):
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

def scope_test(client):
    scope_sampling_rate = 200
    scope_buffer_length = 1000
    print(asyncio.run(client.setup_scope(
        sampling_rate=scope_sampling_rate,
        sample_in_one=True,
        sample_in_two=True,
        sample_out_one=True,
        sample_out_two=True,
        buffer_length=scope_buffer_length,
        adc_active_mode=False
    )))
    print(asyncio.run(client.enable_scope()))


    sleep(scope_buffer_length/scope_sampling_rate)
    
    
    scope_time_axis = np.arange(scope_buffer_length)*1/scope_sampling_rate
    scope_data = asyncio.run(client.get_scope_data())
    if scope_data != False:
        plt.figure()
        for trace_name in scope_data.keys():
            trace = scope_data[trace_name]
            if len(trace) > 0:
                plt.plot(scope_time_axis, trace, label=trace_name)
        
        plt.legend()
        plt.show()

if __name__ == "__main__":
    from os.path import join, dirname
    from time import sleep
    #import numpy as np
    #import matplotlib.pyplot as plt
    # logging.basicConfig(
    #     level=logging.DEBUG,
    #     format="%(asctime)s [%(levelname)s] %(message)s",
    #     handlers=[
    #         # logging.FileHandler("./debug.log"),
    #         logging.StreamHandler()
    #     ]
    # )
    client = SinglePIDClient('192.168.88.200', 10780, 1234)
    #print(asyncio.run(client.initialize(0.01,100000,0,0,10,True, 0, 0, 0.005, True)))

    asyncio.run(client.register_client_id())

    logging.info(f'Successfully initialized Single PID module')
    linearization_file = join(dirname(__file__), 'test_linearization.json')
    linearization_file = 'test_linearization.json'
    linearization_length = 2000
    print(asyncio.run(client.set_ch_one_output_limits(0, 10)))
    # print(asyncio.run(client.set_linearization_length_one(linearization_length)))
    # print(asyncio.run(client.set_linearization_one_from_file(linearization_file)))
    

    #=====Scope Test
    