import asyncio
import logging
from lockstar_client.ScopeClient import ScopeClient
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall


class FGClient(ScopeClient):
    """Basic Module which implements a simple PID controller by using input_1 as error signal,
    input_2 as setpoint and output 1 for the control signal"""

    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, "FGModule")

    async def set_cfunction(self, func):
        bc = BackendCall(
            self.client_id, "FGModule", "set_cfunction", args={"function": func}
        )
        return await self._call_lockstar(bc)

    async def start_ccalculation(self):
        bc = BackendCall(self.client_id, "FGModule", "start_ccalculation", args={})
        return await self._call_lockstar(bc)

    async def start_output(self):
        bc = BackendCall(self.client_id, "FGModule", "start_output", args={})
        return await self._call_lockstar(bc)


client = None


if __name__ == "__main__":
    from os.path import join, dirname
    from time import sleep

    client = FGClient("192.168.137.2", 10780, 1234)

    asyncio.run(client.register_client_id())
    logging.info(f"Successfully initialized Single PID module")

    print(asyncio.run(client.set_ch_one_output_limits(-10, 10)))
    print(asyncio.run(client.set_ch_two_output_limits(-10, 10)))
    print(asyncio.run(client.set_cfunction("arctan")))
    print(asyncio.run(client.start_ccalculation()))
    print(asyncio.run(client.start_output()))