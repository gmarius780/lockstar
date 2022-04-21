import asyncio
import logging
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_general.hardware import HardwareComponents

class LockstarClient():
    def __init__(self, lockstar_ip, lockstar_port, client_id) -> None:
        self.lockstar_ip = lockstar_ip
        self.lockstar_port = lockstar_port
        self.client_id = client_id

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

    def register_client_id(self):
        bc = BackendCall(self.client_id, 'GeneralModule', 'register_client',
                        args={'client_id': self.client_id})
        
        response = asyncio.run(self._call_lockstar(bc))

        if not response.is_ACK():
            logging.warn(f'LockstarClient: could not register my client id: {self.client_id}')

        return response.is_ACK()

    def start_calibration(self, channel: HardwareComponents):
        pass

