import asyncio
import logging
from lockstar_client.ScopeClient import ScopeClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall

class DoubleDitherLockClient(ScopeClient):


    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'CavityLockModule')


    async def set_pid_one(self,  p: float, i: float, d: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_pid_one', args={'p': p, 'i': i, 'd': d})
        return await self._call_lockstar(bc)

    async def set_pid_two(self,  p: float, i: float, d: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_pid_two', args={'p': p, 'i': i, 'd': d})
        return await self._call_lockstar(bc)

    async def lock_one(self):
        bc = BackendCall(self.client_id, self.module_name, 'lock_one', args={})
        return await self._call_lockstar(bc)

    async def lock_two(self):
        bc = BackendCall(self.client_id, self.module_name, 'lock_two', args={})
        return await self._call_lockstar(bc)

    async def unlock_one(self):
        bc = BackendCall(self.client_id, self.module_name, 'unlock_one', args={})
        return await self._call_lockstar(bc)

    async def unlock_two(self):
        bc = BackendCall(self.client_id, self.module_name, 'unlock_two', args={})
        return await self._call_lockstar(bc)

    async def set_dither_one(self,  amp: float, offset: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_dither_one', args={'amp': amp, 'offset': offset})
        return await self._call_lockstar(bc)

    async def set_dither_two(self,  amp: float, offset: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_dither_two', args={'amp': amp, 'offset': offset})
        return await self._call_lockstar(bc)

    async def set_setpoint_one(self,  setpoint: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_setpoint_one', args={'setpoint': setpoint})
        return await self._call_lockstar(bc)

    async def set_setpoint_two(self,  setpoint: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_setpoint_two', args={'setpoint': setpoint})
        return await self._call_lockstar(bc)


if __name__ == "__main__":
    from os.path import join, dirname
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
    client = DoubleDitherLockClient('192.168.88.25', 10780, 1234)

    asyncio.run(client.register_client_id())

    print(asyncio.run(client.set_ch_one_output_limits(-10, 10)))
    print(asyncio.run(client.set_ch_two_output_limits(-10, 10)))
    print(asyncio.run(client.set_dither_one(1, 1)))
    print(asyncio.run(client.set_dither_two(1, 2)))
    

    # print(asyncio.run(client.set_ch_one_output_limits(0, 10)))
    # print(asyncio.run(client.set_linearization_length_one(linearization_length)))
    # print(asyncio.run(client.set_linearization_one_from_file(linearization_file)))
    

    #=====Scope Test
    