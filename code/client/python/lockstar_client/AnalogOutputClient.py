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
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )
    client = AnalogOutputClient('192.168.88.25', 10780, 1234)
    asyncio.run(client.register_client_id())
    
    # Scope Test
    print(asyncio.run(client.setup_scope(
        sampling_rate=5,
        sample_in_one=True,
        sample_in_two=True,
        sample_out_one=True,
        sample_out_two=True,
        buffer_length=100,
        adc_active_mode=True
    )))
    print(asyncio.run(client.enable_scope()))
    print(asyncio.run(client.output_on()))

    for i in range(25):
        print(asyncio.run(client.output_on()))
        print(asyncio.run(client.set_ch_one_output(i*0.04)))
        print(asyncio.run(client.set_ch_two_output(1-i*0.04)))
        sleep(0.6)
    
    print(asyncio.run(client.get_scope_data()))

    print('done')


