import logging
import subprocess
from lockstar_rpi.BackendSettings import BackendSettings
from os.path import join

class Module:
    def __init__(self) -> None:
        pass

    async def call_method(self, backend_call, writer):
        """Calls the method of the module with the name backend_call.method_name and passes the writer as well as 
        backend_call.args to it

        Args:
            backend_call (BackendCall):
            writer (StreamWriter):
        """
        try:
            method_to_call = getattr(self, backend_call.method_name)
            await method_to_call(**backend_call.args, writer=writer)
        except Exception as ex:
            logging.error(f'Module.call_method ({self.__class__.__name__}) cannot call method: {ex}')
            raise ex

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        return {
            'module_name': self.__class__.__name__
        }
    
    def flash_mc(self):
        output = subprocess.run(['sudo','openocd', '-f', join(BackendSettings.elf_directory, f'{self.__class__.__name__}.cfg')],
                                capture_output=True)
        output = subprocess.run(['sudo','openocd', '-f', join(BackendSettings.elf_directory, 'restart.cfg')],
                                capture_output=True)
        
        logging.info(f'Tried flashing mc for module: {self.__class__.__name__} - output: {output}')

    async def launch_from_config(self, config_dict):
        pass