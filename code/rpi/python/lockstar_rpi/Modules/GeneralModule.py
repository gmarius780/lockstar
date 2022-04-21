import logging
from lockstar_rpi.Modules.Module import Module
from lockstar_general.backend.BackendResponse import BackendResponse

class GeneralModule(Module):
    def __init__(self) -> None:
        super().__init__()

        self.client_id = None
    
    async def register_client(self, client_id, writer):
        self.client_id = client_id
        logging.info(f'registered new client id: {client_id}')
        writer.write(BackendResponse.ACK().to_bytes())
        await writer.drain()

if __name__ == "__main__":
    # test module calling
    from lockstar_general.backend.BackendCall import BackendCall
    from lockstar_rpi.ModuleFactory import ModuleFactory
    import asyncio
    bc = BackendCall(123, 'GeneralModule', 'register_client', {'client_id': 2222})
    m = ModuleFactory.I().module_class_form_name(bc.module_name)()
    asyncio.run(m.call_method(bc, 'asdf'))