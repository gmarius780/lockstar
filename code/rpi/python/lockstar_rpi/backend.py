import asyncio
from lockstar_rpi.BackendSettings import BackendSettings
import logging
from lockstar_general.communication.BackendDPFactory import BackendDPFactory
from lockstar_general.communication.BackendModuleTypes import BackendModuleTypes
from lockstar_general.communication.BackendDP import GeneralMethods

class BackendState:
    current_client_id = None
    current_module = None

state_lock = asyncio.Lock()

backend_state = BackendState()

async def handle_client_requests(reader, writer):
    try:
        backend_dp = await BackendDPFactory.get_dp_from_reader(reader)
    except ValueError as ex:
        logging.error(f'handle_client_requests: cannot parse backend_dp: {ex}')
        backend_dp = None
    
    if backend_dp is not None:
        if backend_dp.module_identifier == BackendModuleTypes.GENERAL:
            # === Handle general methods
            if backend_dp.method_identifier == GeneralMethods.INIT_HARDWARE:
                print('init_hardware')
            elif backend_dp.method_identifier == GeneralMethods.FREE_HARDWARE:
                print('free_hardware')
            
        else:
            async with state_lock:
                if backend_state.current_module is not None:
                    if backend_state.current_module.module_identifier == backend_dp.module_identifier:
                        backend_state.current_module.execute_method(backend_dp)
                    else:
                        logging.error(f'handle_client_request: wrong module loaded: loaded module: {backend_state.currently_loaded_module.__class__}, requested module: {backend_dp.module_class}')
                else:
                    logging.error(f'handle_client_request: no module is loaded: requested module: {backend_dp.module_class} init_module first!')

            # if module matches current module --> execute command
            # else: write error response
            pass
    
    

async def main():
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