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

if asyncio.run(client.register_client_id()):
        logging.info(f'Successfully initialized AWG module')
        
        linearization_file = join(dirname(__file__), 'test_linearization.json')
        linearization_length = 2000

        sampling_rate = 500000

        # ch_one_chunks = [999, 1999, 2999, 3999, 4999]
        # ch_two_chunks = [1999, 2999]
        # ch_one_buffer = np.concatenate((np.cos(np.linspace(0, 2*np.pi, num=1000)),
        #                                 np.linspace(0, 1, num=1000),
        #                                 np.cos(np.linspace(0, 6*np.pi, num=1000)),
        #                                 np.linspace(1, -1, num=1000),
        #                                 np.linspace(-1, 0, num=1000)))


        # ch_two_buffer = np.concatenate((np.cos(np.linspace(0, 50*4*np.pi, num=2000)),
        #                                 np.linspace(1, 2, num=500), np.linspace(2, 1, num=500)))

        # ch_one_buffer = (ch_one_buffer * 5).tolist()
        # ch_two_buffer = (ch_two_buffer * 5).tolist()
        
        # print(asyncio.run(client.initialize_buffers(len(ch_one_buffer), len(ch_two_buffer), len(ch_one_chunks), 
        #                                 len(ch_two_chunks), sampling_rate)))
        print(asyncio.run(client.set_sampling_rate(sampling_rate)))
        print(asyncio.run(client.set_ch_one_output_limits(-10, 10)))
        print(asyncio.run(client.set_ch_two_output_limits(-10, 10)))
        print(asyncio.run(client.set_cfnction("arctan")))
        print(asyncio.run(client.start_ccalculation()))
        print(asyncio.run(client.start_output()))