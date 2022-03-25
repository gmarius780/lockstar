from multiprocessing.sharedctypes import Value
from lockstar_general.communication.BackendModuleTypes import  BackendModuleTypes
from enum import Enum
from struct import pack, unpack, calcsize # https://docs.python.org/3/library/struct.html


class GeneralMethods(Enum):
    INIT_HARDWARE = 0
    FREE_HARDWARE = 1

#TODO: all DP's should be implemented in C++
class BackendDP:
    """BackendDataPackage parses input data from any lockstar client according to the following protocol:
    
    2 Bytes             | 1 Byte            | 2 Bytes          | 1 Byte           | 4 Bytes            | 1 Byte           | <N> Bytes                                                     | 1 Byte
    Module-Identifier   |  0xBC (delimiter) |Method-Identifier | 0xBC (delimiter) | Payload-Length (N) | 0xBC (delimiter) | Payload --> will be transfered to the right ModuleDataPackage | 0xBC (delimiter)
    """

    @classmethod
    def for_init_hardware(cls, client_id_int):
        module_identifier_b = BackendModuleTypes.GENERAL.value.to_bytes(2, 'big')
        method_identifier_b = GeneralMethods.INIT_HARDWARE.value.to_bytes(2, 'big')
        payload_b = pack('>i', client_id_int)
        payload_length = calcsize('>i')
        payload_length_b = payload_length.to_bytes(4, 'big')

        return cls(module_identifier_b, method_identifier_b, payload_length_b, payload_b)

    def _initialize_for_init_hardware(self):
        (client_id_int) = unpack('>I', self.payload_b)

        self.method_identifier = GeneralMethods.INIT_HARDWARE

        self.client_id_int = client_id_int


    def __init__(self, module_identifier_b, method_identifier_b, payload_length_b, payload_b) -> None:
        self.module_identifier_b = module_identifier_b
        self.method_identifier_b = method_identifier_b
        self.payload_length = int.from_bytes(payload_length_b, byteorder='big', signed=False)
        self.payload_b = payload_b

        self.method_identifier = None
        self.module_identifier = None
        if self.__class__ == BackendDP:
            self.module_identifier = BackendModuleTypes.GENERAL

            self.client_id_int = None

            # === parse payload
            self.method_identifier = int.from_bytes(method_identifier_b, 'big')

            if self.method_identifier == GeneralMethods.INIT_HARDWARE.value:
                self._initialize_for_init_hardware()
            elif self.method_identifier == GeneralMethods.FREE_HARDWARE.value:
                pass
            else:
                raise ValueError(f'BackendDP: undefined method_identifier: {method_identifier_b}')


    
    def byte_array(self):
        return [
            self.module_identifier_b,
            b'\xBC',
            self.method_identifier_b,
            b'\xBC',
            self.payload_length.to_bytes(4, 'big'),
            b'\xBC',
            self.payload_b,
            b'\xBC'
        ]