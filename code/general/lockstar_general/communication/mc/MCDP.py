from lockstar_general.communication.MCModuleTypes import  MCModuleTypes
from enum import Enum





#TODO: all DP's should be implemented in C++
class MCDP:
    """BackendDataPackage parses input data from any lockstar client according to the following protocol:
    
    2 Bytes             | 1 Byte            | 2 Bytes          | 1 Byte           | 4 Bytes            | 1 Byte           | <N> Bytes                                                     | 1 Byte
    Module-Identifier   |  0xBC (delimiter) |Method-Identifier | 0xBC (delimiter) | Payload-Length (N) | 0xBC (delimiter) | Payload --> will be transfered to the right ModuleDataPackage | 0xBC (delimiter)
    """


    def __init__(self, module_identifier_b, method_identifier_b, payload_length_b, payload_b) -> None:
        self.module_identifier_b = module_identifier_b
        self.method_identifier_b = method_identifier_b
        self.payload_length = int.from_bytes(payload_length_b, byteorder='big', signed=False)
        self.payload_b = payload_b

        self.method_identifier = None
        self.module_identifier = None
    
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