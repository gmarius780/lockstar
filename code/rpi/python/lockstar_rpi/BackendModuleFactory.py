from BackendModules.backend_module_identifiers import BACKEND_MODULE_IDENTIFIERS
import logging


class BackendModuleFactory:

    @staticmethod
    def get_module_class_from_byte_identifier(module_identifier_b):
        if module_identifier_b in BACKEND_MODULE_IDENTIFIERS:
            return BACKEND_MODULE_IDENTIFIERS[module_identifier_b]
        else:
            logging.error(f'BackendModuleFactory: Module identifier does not exist: {module_identifier_b}')
            raise ValueError(f'BackendModuleFactory: Module identifier does not exist: {module_identifier_b}')