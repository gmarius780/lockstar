class BackendModuleDP:
    METHOD_IDENTIFIERS = {}

    @classmethod
    def from_backend_dp(cls, backend_dp):
        return cls(backend_dp.method_identifier_b, backend_dp.payload_length, backend_dp.payload_b)

    def __init__(self, method_identifier_b, payload_length, payload_b) -> None:
        if method_identifier_b in self.__class__.METHOD_IDENTIFIERS:
            self.method = self.__class__.METHOD_IDENTIFIERS[method_identifier_b]
        else:
            raise ValueError(f'BackendModuleDP: {self.__class__.__name__} has no method with identifier {method_identifier_b}')
        
        self.payload_length = payload_length
        self.payload_b = payload_b


class BackendModule:
    
    BACKEND_MODULE_DP_CLASS = BackendModuleDP

    def __init__(self) -> None:
        self.module_dp = None

    def execute_method(self, backend_dp):
        module_dp = self.__class__.BACKEND_MODULE_DP_CLASS.from_backend_dp(backend_dp)

        print(f'execute_method: module_dp: {module_dp.method}')

          