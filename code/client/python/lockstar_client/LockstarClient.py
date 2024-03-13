import asyncio
import logging
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_client.ClientSettings import ClientSettings
import json

class LockstarClient():
    def __init__(self, lockstar_ip, lockstar_port, client_id, module_name) -> None:
        self.lockstar_ip = lockstar_ip
        self.lockstar_port = lockstar_port
        self.client_id = client_id
        self.module_name = module_name

    async def set_ch_one_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_one_output_limits', args={'min': min, 'max': max})
        return await self._call_lockstar(bc)

    async def set_ch_two_output_limits(self, min: float, max: float):
        bc = BackendCall(self.client_id, self.module_name, 'set_ch_two_output_limits', args={'min': min, 'max': max})
        return await self._call_lockstar(bc)
    
    async def unclamp_output(self):
        bc = BackendCall(self.client_id, self.module_name, 'unclamp_output', args={})
        return await self._call_lockstar(bc)    

    #==== Linearization Methods START====
    async def set_linearization_one(self, linearization, min_output_voltage, max_output_voltage):
        """Sets the parameters used by the DAC_ONE to linearize the systems response

        Args:
            linearization (_type_): _description_

        Returns:
            _type_: _description_
        """
        bc = BackendCall(self.client_id, self.module_name, 'set_linearization_one', 
                        args={
                            'linearization': linearization,
                            'min_output_voltage': min_output_voltage,
                            'max_output_voltage': max_output_voltage
                        })
        return await self._call_lockstar(bc)

    async def set_linearization_two(self, linearization, min_output_voltage, max_output_voltage):
        """Sets the parameters used by the DAC_ONE to linearize the systems response

        Args:
            linearization (_type_): _description_

        Returns:
            _type_: _description_
        """
        bc = BackendCall(self.client_id, self.module_name, 'set_linearization_two', 
                        args={
                            'linearization': linearization,
                            'min_output_voltage': min_output_voltage,
                            'max_output_voltage': max_output_voltage
                        })
        return await self._call_lockstar(bc)

    async def set_linearization_two_from_file(self, local_file):
        """ calls set_linearization_two with the linearization gotten from local_file (stored by LinearizationClient.store_linearization_locally)
        """

        with open(local_file, 'r') as f:
            lin_dict = json.load(f)

        return await self.set_linearization_two(linearization=lin_dict['linearization'], 
                                    min_output_voltage=lin_dict['min_output_voltage'],
                                    max_output_voltage=lin_dict['max_output_voltage'])

    async def set_linearization_one_from_file(self, local_file):
        """ calls set_linearization_one with the linearization gotten from local_file (stored by LinearizationClient.store_linearization_locally)
        """

        with open(local_file, 'r') as f:
            lin_dict = json.load(f)

        return await self.set_linearization_one(linearization=lin_dict['linearization'], 
                                    min_output_voltage=lin_dict['min_output_voltage'],
                                    max_output_voltage=lin_dict['max_output_voltage'])

    async def set_linearization_length_one(self, linearization_length: int):
        bc = BackendCall(self.client_id, self.module_name, 'set_linearization_length_one', 
                        args={'linearization_length': linearization_length})
        return await self._call_lockstar(bc)

    async def set_linearization_length_two(self, linearization_length: int):
        bc = BackendCall(self.client_id, self.module_name, 'set_linearization_length_two', 
                        args={'linearization_length': linearization_length})
        return await self._call_lockstar(bc)

    async def enable_linearization_one(self):
        bc = BackendCall(self.client_id, self.module_name, 'enable_linearization_one', args={})
        return await self._call_lockstar(bc)

    async def enable_linearization_two(self):
        bc = BackendCall(self.client_id, self.module_name, 'enable_linearization_two', args={})
        return await self._call_lockstar(bc)

    async def disable_linearization_one(self):
        bc = BackendCall(self.client_id, self.module_name, 'disable_linearization_one', args={})
        return await self._call_lockstar(bc)

    async def disable_linearization_two(self):
        bc = BackendCall(self.client_id, self.module_name, 'disable_linearization_two', args={})
        return await self._call_lockstar(bc)

    async def flash_mc(self):
        bc = BackendCall(self.client_id, self.module_name, 'flash_mc', args={})
        return await self._call_lockstar(bc)
   
    #==== Linearization Methods END====

    async def _call_lockstar(self, backend_call):
        """Send backend_call to lockstar and await response

        Args:
            backend_call (BackendCall): call to send

        Returns:
            BackendResponse: response
        """
        reader, writer = await asyncio.open_connection(self.lockstar_ip, self.lockstar_port, limit=ClientSettings.read_buffer_limit_bytes)
        writer.write(backend_call.to_bytes())
        await writer.drain()
        byte_response = (await reader.readuntil(BackendResponse.DELIMITTER))[:-1]
        response = BackendResponse.from_bytes(byte_response)
        writer.close()
        await writer.wait_closed()
        return response

    async def register_client_id(self):
        bc = BackendCall(self.client_id, 'GeneralModule', 'register_client',
                        args={'client_id': self.client_id})
        
        #response = asyncio.run(self._call_lockstar(bc))
        response = await self._call_lockstar(bc)

        if not response.is_ACK():
            logging.warn(f'LockstarClient: could not register my client id: {self.client_id}')

        return response.is_ACK()


