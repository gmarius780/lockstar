from multiprocessing.sharedctypes import Value
from struct import pack, unpack, calcsize # https://docs.python.org/3/library/struct.html
import logging

from numpy import uint32


class MCDataPackage:
    def __init__(self) -> None:
        self.buffer = []

    @staticmethod
    def struct_type_from_cpp_type(str_cpp_type):
        if str_cpp_type == 'uint32_t': return 'I'
        elif str_cpp_type == 'float': return 'f'
        else: raise ValueError(f'Datatype: {str_cpp_type} not recognized.')

    def push_to_buffer(self, str_cpp_dtype, data):
        try:
            struct_type = MCDataPackage.struct_type_from_cpp_type(str_cpp_dtype)
            self.buffer.append((struct_type, data))
        except ValueError as ex:
            logging.error(ex)
            raise ex

    def get_bytes(self):
        return pack(f'<{"".join([a[0] for a in self.buffer])}', *[a[1] for a in self.buffer])

    def get_nbr_of_bytes(self):
        return calcsize(f'<{"".join([a[0] for a in self.buffer])}')

    @staticmethod
    def pop_from_buffer(list_str_cpp_dtype, lst_bytes):
        list_struct_type = []
        for str_cpp_dtype in list_str_cpp_dtype:
            try:
                struct_type = MCDataPackage.struct_type_from_cpp_type(str_cpp_dtype)
                list_struct_type.append(struct_type)
            except ValueError as ex:
                logging.error(ex)
                raise ex
        
        return unpack(f'<{"".join(list_struct_type)}', lst_bytes)

    @staticmethod
    def get_buffer_length(list_str_cpp_dtype):
        list_struct_type = []
        for str_cpp_dtype in list_str_cpp_dtype:
            try:
                struct_type = MCDataPackage.struct_type_from_cpp_type(str_cpp_dtype)
                list_struct_type.append(struct_type)
            except ValueError as ex:
                logging.error(ex)
                raise ex
        
        return calcsize(f'<{"".join(list_struct_type)}')

    @staticmethod
    def pop_ack_nack_from_buffer(lst_bytes):
        ack_nack = MCDataPackage.pop_from_buffer(['uint32_t'], lst_bytes)[0]

        if ack_nack == 221194:
            return True
        elif ack_nack == 999999:
            return False
        else:
            raise ValueError(f'ack_nack invalid: {ack_nack}')


if __name__ == "__main__":
    test = MCDataPackage()

    test.push_to_buffer('uint32_t', uint32(55))
    test.push_to_buffer('uint32_t', uint32(66))
    test.push_to_buffer('float', float(99.10))

    byte_arr = test.get_bytes()

    list_dtype = ['uint32_t', 'uint32_t', 'float']

    print(f'buffer_length: {MCDataPackage.get_buffer_length(list_dtype)}')
    print(f'buffer: {MCDataPackage.pop_from_buffer(list_dtype, byte_arr)}')