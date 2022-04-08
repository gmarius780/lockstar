from lockstar_general.communication.SinglePIDMCDP import SinglePIDMCDP
from lockstar_general.communication.BackendDP import BackendDP
from lockstar_general.communication.MCModuleTypes import MCModuleTypes

class MCDPFactory:
    """MCDataPackage parses input data from any lockstar client according to the following protocol:
    
    2 Bytes             | 1 Byte            | 2 Bytes          | 1 Byte           | 4 Bytes            | 1 Byte           | <N> Bytes                                                     | 1 Byte
    Module-Identifier   |  0xBC (delimiter) |Method-Identifier | 0xBC (delimiter) | Payload-Length (N) | 0xBC (delimiter) | Payload --> will be transfered to the right ModuleDataPackage | 0xBC (delimiter)
    """
    @staticmethod
    async def get_dp_from_reader(stream_reader):
        """Reads bytes from a stream_reader, makes sure the file format is correct and returns the correct DataPackage instance"""
        # === read module identifier
        module_identifier_b = await stream_reader.readexactly(2)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('MCDP wrong format: no delimitter after module_identifier')

        # === read method identifier

        method_identifier_b = await stream_reader.readexactly(2)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('MCDP wrong format: no delimitter after method_identifier')

        # === read payload length

        payload_length_b = await stream_reader.readexactly(4)
        payload_length = int.from_bytes(payload_length_b, byteorder='big', signed=False)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('MCDP wrong format: no delimitter after payload_length')

        # === read payload

        payload_b = await stream_reader.readexactly(payload_length)

        if not await stream_reader.readexactly(1) == b'\xBC':
            raise ValueError('MCDP wrong format: no delimitter after payload')
        
        return MCDPFactory.get_dp(module_identifier_b, method_identifier_b, payload_length_b, payload_b)


    @staticmethod
    def get_dp(module_identifier_b, method_identifier_b, payload_length_b, payload_b):
        """Returns the correct DP instance corresponding to module_identifier
        THIS SHOULD BE OUTSOURCED INTO C++"""
        
        module_identifier = int.from_bytes(module_identifier_b, 'big')

        if module_identifier == MCModuleTypes.GENERAL.value:
            return BackendDP(module_identifier_b, method_identifier_b, payload_length_b, payload_b)
        elif module_identifier == MCModuleTypes.SINGLEPID.value:
            return SinglePIDMCDP(module_identifier_b, method_identifier_b, payload_length_b, payload_b)
        else:
            raise ValueError(f'MCDPFactory: unsupported module identifier: {module_identifier_b}')
