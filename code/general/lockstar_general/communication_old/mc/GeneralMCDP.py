from lockstar_general.communication.MCDP import MCDP
from lockstar_general.communication.MCModuleTypes import  MCModuleTypes
from struct import pack, unpack, calcsize # https://docs.python.org/3/library/struct.html
from enum import Enum


class GeneralMethods(Enum):
    INIT_HARDWARE = 0
    FREE_HARDWARE = 1

class GeneralMCDP(MCDP):
    """BackendDataPackage parses input data from any lockstar client according to the following protocol:
    
    2 Bytes             | 1 Byte            | 2 Bytes          | 1 Byte           | 4 Bytes            | 1 Byte           | <N> Bytes                                                     | 1 Byte
    Module-Identifier   |  0xBC (delimiter) |Method-Identifier | 0xBC (delimiter) | Payload-Length (N) | 0xBC (delimiter) | Payload --> will be transfered to the right ModuleDataPackage | 0xBC (delimiter)
    """

    @classmethod
    def init_hardware(cls, client_id_int):
        module_identifier_b = MCModuleTypes.GENERAL.value.to_bytes(2, 'big')
        method_identifier_b = GeneralMethods.INIT_HARDWARE.value.to_bytes(2, 'big')
        payload_b = pack('>i', client_id_int)
        payload_length = calcsize('>i')
        payload_length_b = payload_length.to_bytes(4, 'big')

        return cls(module_identifier_b, method_identifier_b, payload_length_b, payload_b)

    def _initialize_init_hardware(self):
        (client_id_int) = unpack('>i', self.payload_b)

        self.method_identifier = GeneralMethods.INIT_HARDWARE

        self.client_id_int = client_id_int


    def __init__(self, module_identifier_b, method_identifier_b, payload_length_b, payload_b) -> None:
        super(GeneralMCDP).__init__(module_identifier_b, method_identifier_b, payload_length_b, payload_b)
        self.module_identifier = MCModuleTypes.GENERAL
        self.client_id_int = None

        # === parse payload
        self.method_identifier = int.from_bytes(method_identifier_b, 'big')

        if self.method_identifier == GeneralMethods.INIT_HARDWARE.value:
            self._initialize_for_init_hardware()
        elif self.method_identifier == GeneralMethods.FREE_HARDWARE.value:
            pass
        else:
            raise ValueError(f'BackendDP: undefined method_identifier: {method_identifier_b}')
