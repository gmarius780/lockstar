import asyncio
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall

class BufferBaseClient_(LockstarClient):
    def __init__(self, lockstar_ip, lockstar_port, client_id, module_name) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, module_name)


    def initialize(self):
        pass
    
    def output_on(self):
        bc = BackendCall(self.client_id, self.module_name, 'output_on', args={})
        return asyncio.run(self._call_lockstar(bc))

    def output_off(self):
        bc = BackendCall(self.client_id, self.module_name, 'output_off', args={})
        return asyncio.run(self._call_lockstar(bc))

    def output_ttl(self):
        bc = BackendCall(self.client_id, self.module_name, 'output_ttl', args={})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_one_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_two_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_two_output_limits', args={'min': min, 'max': max})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_buffer(self, buffer):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_one_buffer', args={'buffer': buffer})
        return asyncio.run(self._call_lockstar(bc))
    
    def set_ch_two_buffer(self, buffer):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_two_buffer', args={'buffer': buffer})
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
        bc = BackendCall(self.client_id, self.module_name, 'initialize_buffers', 
                        args={'buffer_one_size': buffer_one_size, 'buffer_two_size': buffer_two_size,
                        'chunks_one_size': chunks_one_size, 'chunks_two_size': chunks_two_size, 'sampling_rate': sampling_rate})
        return asyncio.run(self._call_lockstar(bc))

    def set_sampling_rate(self, sampling_rate: int):
        bc = BackendCall(self.client_id, self.module_name, 'set_sampling_rate', args={'sampling_rate': sampling_rate,})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_one_chunks(self, chunks):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_one_chunks', args={'chunks': chunks})
        return asyncio.run(self._call_lockstar(bc))

    def set_ch_two_chunks(self, chunks):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_two_chunks', args={'chunks': chunks})
        return asyncio.run(self._call_lockstar(bc))


