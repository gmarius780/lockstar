from BackendModuleFactory import BackendModuleFactory


class BackendDP:
    """BackendDataPackage parses input data from any lockstar client according to the following protocol:
    
    2 Bytes             | 1 Byte            | 2 Bytes          | 1 Byte           | 4 Bytes            | 1 Byte           | <N> Bytes                                                     | 1 Byte
    Module-Identifier   |  0xBC (delimiter) |Method-Identifier | 0xBC (delimiter) | Payload-Length (N) | 0xBC (delimiter) | Payload --> will be transfered to the right ModuleDataPackage | 0xBC (delimiter)
    """
    
    @classmethod
    async def from_stream_reader(cls, stream_reader):
        # === read module identifier
        module_identifier_b = await stream_reader.readexactly(2)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('BackendDP wrong format: no delimitter after module_identifier')

        # === read method identifier

        method_identifier_b = await stream_reader.readexactly(2)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('BackendDP wrong format: no delimitter after method_identifier')

        # === read payload length

        payload_length_b = await stream_reader.readexactly(4)
        payload_length = int.from_bytes(payload_length_b, byteorder='big', signed=False)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('BackendDP wrong format: no delimitter after payload_length')

        # === read payload

        payload_b = await stream_reader.readexactly(payload_length)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('BackendDP wrong format: no delimitter after payload')

        return cls(module_identifier_b, method_identifier_b, payload_length_b, payload_b)

    def __init__(self, module_identifier_b, method_identifier_b, payload_length_b, payload_b) -> None:
        self.module_identifier_b = module_identifier_b
        self.method_identifier_b = method_identifier_b
        self.payload_length = int.from_bytes(payload_length_b, byteorder='big', signed=False)
        self.payload_b = payload_b

        self.module_class = None

    def is_general_method_call(self):
        """if module_identifier_b == 0x00, the DP tries to call a general method like init_hardware"""
        if int.from_bytes(self.module_identifier_b, byteorder='big') == 0:
            return True
        else: 
            return False
        
    def module_class(self):
        if self.is_general_method_call():
            return None
        else:
            if self.module_class is None:
                self.module_class = BackendModuleFactory.get_module_class_from_byte_identifier(self.module_identifier_b)
            
            return self.module_class
    
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