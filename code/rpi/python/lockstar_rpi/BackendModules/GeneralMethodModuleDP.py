from BackendModules.BackendModule import BackendModuleDP

class GeneralMethodModuleDP(BackendModuleDP):

    METHOD_IDENTIFIERS = {
        int(1).to_bytes(2, 'big'): 'init_hardware',
        int(2).to_bytes(2, 'big'): 'free_hardware',
        int(3).to_bytes(2, 'big'): 'init_module',
    }
