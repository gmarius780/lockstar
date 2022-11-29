import asyncio
import logging
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall

class LockstarClient():
    def __init__(self, lockstar_ip, lockstar_port, client_id, module_name) -> None:
        self.lockstar_ip = lockstar_ip
        self.lockstar_port = lockstar_port
        self.client_id = client_id
        self.module_name = module_name


    #==== Linearization Methods START====
    def set_linearization_one(self, linearization):
        """Sets the parameters used by the DAC_ONE to linearize the systems response

        Args:
            linearization (_type_): _description_

        Returns:
            _type_: _description_
        """
        bc = BackendCall(self.client_id, self.module_name, 'set_linearization_one', args={})
        return asyncio.run(self._call_lockstar(bc))

    def linearize_one(self, ramp_start, ramp_end, nbr_of_ramp_points):
        """starts the linearization procedure of the microcontroller.

        CAN BE BLOCKING for the backend????:
            1. MC starts ramp & records the trace
            2. only sends ack when done
            3. backend calls 'get_trace_one'
            4. backend calculates linearization
            5. backend calls 'set_linearization_one'
            6. backend returns the trace, and the calculated linearization
        Args:
            ramp_start (_type_): _description_
            ramp_end (_type_): _description_
            nbr_of_ramp_points (_type_): _description_

        Returns:
            _type_: _description_
        """

        bc = BackendCall(self.client_id, self.module_name, 'start_linearization_one', args={})
        return asyncio.run(self._call_lockstar(bc))

    #==== Linearization Methods END====

    async def _call_lockstar(self, backend_call):
        """Send backend_call to lockstar and await response

        Args:
            backend_call (BackendCall): call to send

        Returns:
            BackendResponse: response
        """
        reader, writer = await asyncio.open_connection(self.lockstar_ip, self.lockstar_port)
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
        
        response = asyncio.run(self._call_lockstar(bc))

        if not response.is_ACK():
            logging.warn(f'LockstarClient: could not register my client id: {self.client_id}')

        return response.is_ACK()

    def start_calibration(self):
        pass

