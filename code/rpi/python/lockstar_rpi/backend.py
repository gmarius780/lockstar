import asyncio
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_rpi.ModuleFactory import ModuleFactory
from lockstar_rpi.Modules.GeneralModule import GeneralModule
import logging
import json
import os


class BackendState:
    current_module = None
    general_module = None

state_lock = asyncio.Lock()

backend_state = BackendState()

async def handle_client_requests(reader, writer):
    """This method is called for each TCP/IP request, the server gets

    Args:
        reader (_type_): _description_
        writer (_type_): _description_
    """
    backend_call = None
    try:
        byte_backend_call = (await reader.readuntil(separator=b'\xBC'))[:-1]
        backend_call = BackendCall.from_bytes(byte_backend_call)
    except Exception as ex:
        logging.error(f'handle_client_requests: cannot parse backend_dp: {ex}')
        backend_call = None
    
    if backend_call is not None:
        client_id_missmatch = False
        async with state_lock:
            if backend_state.general_module.client_id != backend_call.client_id and backend_state.general_module.client_id is not None: ##S
                client_id_missmatch = True

        if client_id_missmatch and not (backend_call.module_name == 'GeneralModule' and backend_call.method_name == 'register_client'):
            logging.error(f'Currently a client with another id is using the device: {backend_state.general_module.client_id} (your id: {backend_call.client_id}')
            response = BackendResponse.wrong_client_id()
            writer.write(response.to_bytes())
            await writer.drain()
        else:
            if backend_call.module_name == 'GeneralModule':
                async with state_lock:
                    await backend_state.general_module.call_method(backend_call, writer)
            else:
                async with state_lock:
                    # instantiate new module if necessary
                    if backend_state.current_module is None or backend_state.current_module.__class__.__name__ != backend_call.module_name:
                        backend_state.current_module = ModuleFactory.I().module_class_form_name(backend_call.module_name)()
                        # flash MC
                        await backend_state.current_module.flash_mc()
                        

                    await backend_state.current_module.call_method(backend_call, writer)
        
        # write config of current module
        async with state_lock:
            if backend_state.current_module is not None:
                config_dict = backend_state.current_module.generate_config_dict()
                with open(BackendSettings.current_module_config_file, 'w+') as config_file:
                    json.dump(config_dict, config_file)
    

async def main():
    """
    Opens a TCP/IP service which listens to address and port defined in BackendSettings.
    If there is an existing config file (name: BackendSettings.currend_module_config_file) it will
    launch this module such that the lockstar operates with the same settings as before the shutdown
    """
    # initialize GeneralModule
    backend_state.general_module = GeneralModule()

    # initialize current module if config file is available
    if os.path.exists(BackendSettings.current_module_config_file):
        with open(BackendSettings.current_module_config_file, 'r') as config_file:
            config_dict = json.load(config_file)

            try:
                backend_state.current_module = ModuleFactory.I().module_class_form_name(config_dict['module_name'])() ## S: should be "from_name"?
                # await backend_state.current_module.flash_mc()
                await backend_state.current_module.launch_from_config(config_dict)

            except Exception as ex:
                logging.error(f'Could not launch from config file: {ex} (config_dict: {config_dict})')
                backend_state.current_module = None

    server = await asyncio.start_server(
        handle_client_requests, BackendSettings.backend_ip, BackendSettings.backend_port, limit=BackendSettings.read_buffer_limit_bytes)

    addr = server.sockets[0].getsockname()
    logging.info(f'Serving on {addr}')

    async with server:
        await server.serve_forever()


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )

    asyncio.run(main(), debug=True)
