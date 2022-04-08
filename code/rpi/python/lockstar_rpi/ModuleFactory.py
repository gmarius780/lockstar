import logging
import os
from glob import glob
import sys
import inspect
from lockstar_rpi.Modules.Module import Module

class ModuleFactory():
    """Provides the link between a module name and the Module class"""
    __instance = None

    @staticmethod
    def I():
        if ModuleFactory.__instance is None:
            # dynamically import all Modules
            module_class_dict = {}

            modules_directory = os.path.join(os.path.dirname(__file__), 'Modules')
            # import all python files ending with Modules.py in the Modules directory,
            # for each file get the class with the same name as the file
            for modules_file in glob(os.path.join(modules_directory, '*Module.py')):
                python_module_name = os.path.splitext(os.path.basename(modules_file))[0]
                python_module_name = f'lockstar_rpi.Modules.{python_module_name}'
                __import__(python_module_name)
                
                for name, obj in inspect.getmembers(sys.modules[python_module_name]):
                    # if not (name.startswith('__') or name.endswith('__')):
                    #     if name.endswith('Module'):
                    if f'lockstar_rpi.Modules.{name}' == python_module_name:
                        if inspect.isclass(obj):
                            module_class_dict[name] = obj

            ModuleFactory.__instance = ModuleFactory(module_class_dict)
        return ModuleFactory.__instance
    
    def __init__(self, module_class_dict) -> None:
        self.module_class_dict = module_class_dict

    def module_class_form_name(self, module_name):
        """Returns the Module class corresponding to a string containing the ModuleClass name"""
        if module_name not in self.module_class_dict:
            logging.error(f'Modulefactory: {module_name} does not exist')
            raise ValueError(f'Modulefactory: {module_name} does not exist')
        else:
            return self.module_class_dict[module_name]
