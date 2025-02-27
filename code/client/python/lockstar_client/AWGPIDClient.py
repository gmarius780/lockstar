import asyncio
import logging
from lockstar_client.BufferBaseClient_ import BufferBaseClient_
from lockstar_general.backend.BackendCall import BackendCall


class AWGPIDClient(BufferBaseClient_):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'AWGPIDModule')

    async def set_pid_one(self,  p: float, i: float, d: float, input_offset: float, output_offset: float, i_threshold: float):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'set_pid_one', args={'p': p, 'i': i, 'd': d, 
                    'input_offset': input_offset, 'output_offset': output_offset, 'i_threshold': i_threshold})
        return await self._call_lockstar(bc)

    async def set_pid_two(self,  p: float, i: float, d: float, input_offset: float, output_offset: float, i_threshold: float):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'set_pid_two', args={'p': p, 'i': i, 'd': d, 
                    'input_offset': input_offset, 'output_offset': output_offset, 'i_threshold': i_threshold})
        return await self._call_lockstar(bc)

    async def lock(self):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'lock', args={})
        return await self._call_lockstar(bc)

    async def unlock(self):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'unlock', args={})
        return await self._call_lockstar(bc)
    
    async def enable_intensity_lock_mode_one(self):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'enable_intensity_lock_mode_one', args={})
        return await self._call_lockstar(bc)

    async def disable_intensity_lock_mode_one(self):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'disable_intensity_lock_mode_one', args={})
        return await self._call_lockstar(bc)
    
    async def enable_intensity_lock_mode_two(self):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'enable_intensity_lock_mode_two', args={})
        return await self._call_lockstar(bc)

    async def disable_intensity_lock_mode_two(self):
        bc = BackendCall(self.client_id, 'AWGPIDModule', 'disable_intensity_lock_mode_two', args={})
        return await self._call_lockstar(bc)

if __name__ == "__main__":
    import numpy as np
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )
    client = AWGPIDClient('192.168.88.25', 10780, 1234)

    
    if client.register_client_id():
        logging.info(f'Successfully initialized AWG module')

        sampling_rate = 1000
        
        ch_one_chunks = [999, 1999, 2999, 3999, 4999]
        ch_two_chunks = [1999, 2999]
        # ch_one_buffer = np.concatenate((np.sin(np.linspace(0, 2*np.pi, num=1000)),
        #                                 np.linspace(0, 1, num=1000),
        #                                 np.cos(np.linspace(0, 6*np.pi, num=1000)),
        #                                 np.linspace(1, -3, num=1000),
        #                                 np.linspace(-3, 0, num=1000))).tolist()
        ch_one_buffer = np.concatenate((np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
                                        np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
                                        np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
                                        np.sin(np.linspace(0, 1*2*np.pi, num=1000)),
                                        np.sin(np.linspace(0, 1*2*np.pi, num=1000)))).tolist()

        ch_two_buffer = np.concatenate((np.cos(np.linspace(0, 50*4*np.pi, num=2000)),
                                        np.linspace(1, 4, num=500), np.linspace(4, 1, num=500))).tolist()
        

        # ch_one_chunks = [1999]
        # ch_two_chunks = [1999]
        
        # ch_one_buffer = np.sin(np.linspace(0, 50*2*np.pi, num=2000)).tolist()
        # ch_two_buffer = np.sin(np.linspace(0, 50*2*np.pi, num=2000)).tolist()


        # print(client.set_ch_one_output_limits(0, 1))
        print(asyncio.run(client.initialize_buffers(len(ch_one_buffer), len(ch_two_buffer), len(ch_one_chunks), 
                                        len(ch_two_chunks), sampling_rate)))
        print(asyncio.run(client.set_ch_one_output_limits(-5, 5)))
        print(asyncio.run((client.set_ch_two_output_limits(-5, 5))))
        print(asyncio.run(client.set_ch_one_chunks(ch_one_chunks)))
        print(asyncio.run(client.set_ch_two_chunks(ch_two_chunks)))
        print(asyncio.run(client.set_ch_one_buffer(ch_one_buffer)))
        print(asyncio.run(client.set_ch_two_buffer(ch_two_buffer)))
        print(asyncio.run(client.set_pid_one(1,1,0)))
        print(asyncio.run(client.set_pid_two(1,0,0)))
        print(asyncio.run(client.output_ttl()))

