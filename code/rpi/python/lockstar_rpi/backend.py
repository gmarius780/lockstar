import asyncio
from BackendSettings import BackendSettings
from code.general.communication.BackendDP import BackendDP
import logging
from BackendModules.GeneralMethodModuleDP import GeneralMethodModuleDP

class BackendState:
    currently_loaded_module = None

state_lock = asyncio.Lock()

backend_state = BackendState()

async def handle_client_requests(reader, writer):
    try:
        backend_dp = await BackendDP.from_stream_reader(reader)
    except ValueError as ex:
        logging.error(f'handle_client_requests: cannot parse backend_dp: {ex}')
        backend_dp = None
    
    if backend_dp is not None:
        if backend_dp.is_general_method_call():
            # === Handle general methods
            try:
                module_dp = GeneralMethodModuleDP.from_backend_dp(backend_dp)
            except ValueError as ex:
                logging.error(f'Could not parse BackendModuleDP: {ex}')
                module_dp = None

            if module_dp is not None:
                if module_dp.method == 'init_hardware':
                    print('init_hardware')
                elif module_dp.method == 'free_hardware':
                    print('free_hardware')
                elif module_dp.method == 'init_module':
                    print('init_module')
            
        else:
            async with state_lock:
                if backend_state.currently_loaded_module is not None:
                    if isinstance(backend_state.currently_loaded_module, backend_dp.module_class):
                        backend_state.currently_loaded_module.execute_method(backend_dp)
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