from BackendModules.SinglePIDBackendModule import SinglePIDBackendModule


BACKEND_MODULE_IDENTIFIERS = {
    int(1).to_bytes(2, 'big'): SinglePIDBackendModule
}