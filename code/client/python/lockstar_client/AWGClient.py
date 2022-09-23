import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall

class AWGClient(LockstarClient):
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id)


    def initialize(self):
        pass
    
    def output_on(self):
        bc = BackendCall(self.client_id, 'AWGModule', 'output_on', args={})
        return asyncio.run(self._call_lockstar(bc))

    def output_off(self):
        bc = BackendCall(self.client_id, 'AWGModule', 'output_off', args={})
        return asyncio.run(self._call_lockstar(bc))

    def output_ttl(self):
        bc = BackendCall(self.client_id, 'AWGModule', 'output_ttl', args={})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_ch_one_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_two_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_ch_two_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_buffer(self, buffer):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_ch_one_buffer', args={'buffer': buffer})
        return asyncio.run(self._call_lockstar(bc))
    
    def set_ch_two_buffer(self, buffer):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_ch_two_buffer', args={'buffer': buffer})
        return asyncio.run(self._call_lockstar(bc))

    def initialize_buffers(self, buffer_one_size: int, buffer_two_size: int, chunks_one_size: int, 
                            chunks_two_size: int, sampling_rate:int):
        """ buffer_one_size + buffer_two_size <= 50000

        Args:
            buffer_one_size (int): number of floats for channel one buffer
            buffer_two_size (int): number of floats for channel two buffer
            chunks_one_size (int): number of chunks in channel one buffer
            chunks_two_size (int): number of chunks in channel two buffer
            sampling_rate (int): sampling rate in Hz

        Returns:
            BackendResponse: response
        """
        bc = BackendCall(self.client_id, 'AWGModule', 'initialize_buffers', 
                        args={'buffer_one_size': buffer_one_size, 'buffer_two_size': buffer_two_size,
                        'chunks_one_size': chunks_one_size, 'chunks_two_size': chunks_two_size, 'sampling_rate': sampling_rate})
        return asyncio.run(self._call_lockstar(bc))

    def set_sampling_rate(self, sampling_rate: int):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_sampling_rate', args={'sampling_rate': sampling_rate,})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_chunks(self, chunks):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_ch_one_chunks', args={'chunks': chunks})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_two_chunks(self, chunks):
        bc = BackendCall(self.client_id, 'AWGModule', 'set_ch_two_chunks', args={'chunks': chunks})
        return asyncio.run(self._call_lockstar(bc))


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
    client = AWGClient('192.168.88.13', 10780, 1234)

    
    if client.register_client_id():
        logging.info(f'Successfully initialized AWG module')

        buffer_one_size = buffer_two_size = 25e3
        sampling_rate = 1000
        
        ch_one_chunks = [999, 4999, 7999, 11999, 24999]
        ch_two_chunks = [4999, 9999, 14999, 19999, 24999]
        
        ch_one_buffer = np.concatenate((np.sin(np.linspace(0, 10, num=1000)),
                                        np.linspace(1, 5, num=4000),
                                        np.linspace(5, 1, num=3000),
                                        np.cos(np.linspace(0, 10, num=4000)),
                                        -2*np.ones(13000)))

        ch_two_buffer = np.concatenate((np.sin(np.linspace(0, 10, num=5000)),
                                        np.linspace(1, 5, num=5000),
                                        np.linspace(5, 1, num=5000),
                                        np.cos(np.linspace(0, 10, num=5000),
                                        -2*np.ones(5000))))

        print(client.initialize_buffers(int(buffer_one_size), int(buffer_two_size), 10, 10, sampling_rate))
        # client.set_ch_one_output_limits(-5, 5)
        # client.set_ch_two_output_limits(-5, 5)
        # client.set_ch_one_chunks(ch_one_chunks)
        # client.set_ch_two_chunks(ch_two_chunks)
        # client.set_ch_one_buffer(ch_one_buffer)
        # client.set_ch_two_buffer(ch_two_buffer)
        # client.output_ttl()

