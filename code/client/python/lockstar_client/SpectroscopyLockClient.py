import asyncio
import logging
from lockstar_client.ScopeClient import ScopeClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall

class SpectroscopyLockClient(ScopeClient):


    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'SpectroscopyLockModule')


    async def set_pid(self,  p: float, i: float, d: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_pid', args={'p': p, 'i': i, 'd': d})
        return await self._call_lockstar(bc)
   
    async def lock(self):
        bc = BackendCall(self.client_id, self.module_name, 'lock', args={})
        return await self._call_lockstar(bc)


    async def unlock(self):
        bc = BackendCall(self.client_id, self.module_name, 'unlock', args={})
        return await self._call_lockstar(bc)


    async def set_dither(self,  amp: float, offset: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_dither', args={'amp': amp, 'offset': offset})
        return await self._call_lockstar(bc)


    async def set_dither_frq(self,  dither_frq: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_dither_frq', args={'dither_frq': dither_frq})
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
    client = SpectroscopyLockClient('192.168.88.25', 10780, 1234)

    asyncio.run(client.register_client_id())

    