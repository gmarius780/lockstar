import asyncio
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall

class ScopeClient(LockstarClient):
    def __init__(self, lockstar_ip, lockstar_port, client_id, module_name) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, module_name)

    async def enable_scope(self):
        bc = BackendCall(self.client_id, self.module_name, 'enable_scope', args={})
        return await self._call_lockstar(bc)

    async def disable_scope(self):
        bc = BackendCall(self.client_id, self.module_name, 'enable_scope', args={})
        return await self._call_lockstar(bc)

    async def get_scope_data(self):
        bc = BackendCall(self.client_id, self.module_name, 'get_scope_data', args={})
        br = await self._call_lockstar(bc)
        return br.response

    async def set_scope_sampling_rate(self, sampling_rate: int):
        bc = BackendCall(self.client_id, self.module_name, 'set_scope_sampling_rate', args={
            'sampling_rate': sampling_rate
        })
        return await self._call_lockstar(bc)

    async def setup_scope(self, sampling_rate: int, sample_in_one: bool, sample_in_two: bool, \
        sample_out_one: bool, sample_out_two: bool, buffer_length: int, adc_active_mode: bool):
        """Sets up the scope such that recorded data can be querried by calling scope_get_data. Data can be recorded from
        all analog inputs and outputs by putting the appropriate sample_<in/out>_<one_two> to true. The number of samples
        per channel can be set by <buffer_length> and the sampling rate by <sampling_rate>. If adc_active_mode==True, the scope
        calls the adc->start_conversion function itself according to the sampling rate. Otherwise, the scope simply returns
        the last recorded value, this uses less resources and can be useful if one wants to 'see what the workloop sees'.

        Args:
            sampling_rate (int): scope sampling rate (positive integer (Hz))
            sample_in_one (bool): whether or not to record the corresponding channel
            sample_in_two (bool): whether or not to record the corresponding channel
            sample_out_one (bool): whether or not to record the corresponding channel
            sample_out_two (bool): whether or not to record the corresponding channel
            buffer_length (int): number of floats per activated channel to be recorded
            adc_active_mode (bool): whether or not the scope should call adc->start_conversion itself
        """
        bc = BackendCall(self.client_id, self.module_name, 'setup_scope', args={
            'sampling_rate': sampling_rate, 'sample_in_one': sample_in_one, 'sample_in_two': sample_in_two,
            'sample_out_one': sample_out_one, 'sample_out_two': sample_out_two, 'buffer_length': buffer_length,
            'adc_active_mode': adc_active_mode
        })
        return await self._call_lockstar(bc)