import asyncio
import logging
from lockstar_client.ScopeClient import ScopeClient
from lockstar_general.backend.BackendCall import BackendCall

class AnalogOutputClient(ScopeClient):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'AnalogOutputModule')

    async def initialize(self):
        pass
    
    async def output_on(self):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'output_on', args={})
        return await self._call_lockstar(bc)

    async def output_off(self):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'output_off', args={})
        return await self._call_lockstar(bc)

    async def output_ttl(self):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'output_ttl', args={})
        return await self._call_lockstar(bc)

    async def set_ch_one_output(self, value: float):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'set_ch_one_output', args={'value': value})
        return await self._call_lockstar(bc)
    
    async def set_ch_two_output(self, value: float):
        bc = BackendCall(self.client_id, 'AnalogOutputModule', 'set_ch_two_output', args={'value': value})
        return await self._call_lockstar(bc)


if __name__ == "__main__":
    from time import sleep
    import numpy as np
    import matplotlib.pyplot as plt
    # logging.basicConfig(
    #     level=logging.DEBUG,
    #     format="%(asctime)s [%(levelname)s] %(message)s",
    #     handlers=[
    #         # logging.FileHandler("./debug.log"),
    #         logging.StreamHandler()
    #     ]
    # )
    client = AnalogOutputClient('192.168.88.25', 10780, 1234)
    asyncio.run(client.register_client_id())
    
    # Scope Test
    # print(asyncio.run(client.disable_scope()))
    scope_sampling_rate = 50
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
    print(asyncio.run(client.output_on()))

    for i in range(5):
        print(asyncio.run(client.output_on()))
        print(asyncio.run(client.set_ch_one_output(i*0.1)))
        print(asyncio.run(client.set_ch_two_output(1-i*0.1)))
        sleep(0.6)
    
    
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
    



    print('done')


