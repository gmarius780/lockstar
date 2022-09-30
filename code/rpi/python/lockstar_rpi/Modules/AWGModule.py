from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
from fractions import Fraction

class AWGModule(IOModule_):
    BUFFER_LIMIT_kBYTES = 180
    MAX_NBR_OF_CHUNKS = 100


    def __init__(self) -> None:
        super().__init__()
        self.is_output_on = False
        self.is_output_ttl = False
        self.out_range_ch_one_min = 0.
        self.out_range_ch_one_max = 0.
        self.out_range_ch_two_min = 0.
        self.out_range_ch_two_max = 0.
        self.buffer_one = None
        self.buffer_two = None
        self.buffer_one_size = 0
        self.buffer_two_size = 0
        self.chunks_one_size = 0
        self.chunks_two_size = 0
        self.sampling_rate = 0

    # ==== START: client methods 
    async def initialize(self, writer):
        pass

    async def output_on(self, writer, respond=True):
        logging.debug('Backend: output_on')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))
    
    async def output_off(self, writer, respond=True):
        logging.debug('Backend: output_off')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def output_ttl(self, writer, respond=True):
        logging.debug('Backend: output_ttl')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_one_output_limits(self, min: float, max: float, writer, respond=True):
        self.out_range_ch_one_min = min
        self.out_range_ch_one_max = max

        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_two_output_limits(self, min: float, max: float, writer, respond=True):
        self.out_range_ch_two_min = min
        self.out_range_ch_two_max = max

        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 15) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_one_buffer(self, buffer, writer, respond=True):
        if len(buffer) > self.buffer_one_size:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            self.buffer_one = buffer

            logging.debug('Backend: set ch one buffer')
            # send buffer in chunks of MCDataPackage.MAX_NBR_BYTES
            for i in range(0, len(buffer), MCDataPackage.MAX_NBR_BYTES - 64):
                logging.debug('send chunk')
                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t', 16) # method_identifier
                if i == 0:
                    mc_data_package.push_to_buffer('bool', False) #overwrite buffer
                else:
                    mc_data_package.push_to_buffer('bool', True) #append to buffer
                    
                    nbr_values_to_read = MCDataPackage.MAX_NBR_BYTES - 64
                    if i + MCDataPackage.MAX_NBR_BYTES - 64 > len(buffer):
                        nbr_values_to_read = len(buffer) - i
                    
                    mc_data_package.push_to_buffer('uint32_t', nbr_values_to_read)
                for f in buffer[i:i+MCDataPackage.MAX_NBR_BYTES]: #doesn't matter if index is too large
                    mc_data_package.push_to_buffer('float', f)
                await MC.I().write_mc_data_package(mc_data_package)
            return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_two_buffer(self, buffer, writer, respond=True):
        if len(buffer) > self.buffer_two_size:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            self.buffer_two = buffer

            logging.debug('Backend: set ch two buffer')
            # send buffer in chunks of MCDataPackage.MAX_NBR_BYTES
            for i in range(0, len(buffer), MCDataPackage.MAX_NBR_BYTES):
                logging.debug('send chunk')
                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t', 17) # method_identifier
                if i == 0:
                    mc_data_package.push_to_buffer('bool', False) #overwrite buffer
                else:
                    mc_data_package.push_to_buffer('bool', True) #append to buffer
                for f in buffer[i:i+MCDataPackage.MAX_NBR_BYTES]: #doesn't matter if index is too large
                    mc_data_package.push_to_buffer('float', f)
                await MC.I().write_mc_data_package(mc_data_package)
            return await self.check_for_ack(writer=(writer if respond else None))

    async def initialize_buffers(self, buffer_one_size: int, buffer_two_size: int, chunks_one_size: int, 
                                chunks_two_size: int, sampling_rate:int, writer, respond=True):
        """_summary_

        Args:
            buffer_one_size (int): number of floats for channel one buffer
            buffer_two_size (int): number of floats for channel two buffer
            chunks_one_size (int): number of chunks in channel one buffer
            chunks_two_size (int): number of chunks in channel two buffer
            sampling_rate (int): sampling rate in Hz
            writer (_type_): _description_
            respond (bool, optional): _description_. Defaults to True.

        Returns:
            _type_: _description_
        """
        if buffer_one_size + buffer_two_size > AWGModule.BUFFER_LIMIT_kBYTES*250:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        elif chunks_one_size + chunks_two_size > AWGModule.MAX_NBR_OF_CHUNKS:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            fraction = Fraction.from_float(sampling_rate / BackendSettings.mc_internal_clock_rate)
            fraction = fraction.limit_denominator(BackendSettings.mc_max_prescaler)
            prescaler = fraction.denominator
            counter_max = fraction.numerator

            self.buffer_one_size = buffer_one_size
            self.buffer_two_size = buffer_two_size
            self.chunks_one_size = chunks_one_size
            self.chunks_two_size = chunks_two_size
            self.sampling_rate = sampling_rate

            logging.debug('Backend: initialize_buffers')
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 18) # method_identifier
            mc_data_package.push_to_buffer('uint32_t', buffer_one_size)
            mc_data_package.push_to_buffer('uint32_t', buffer_two_size)
            mc_data_package.push_to_buffer('uint32_t', chunks_one_size)
            mc_data_package.push_to_buffer('uint32_t', chunks_two_size)
            mc_data_package.push_to_buffer('uint32_t', int(prescaler))
            mc_data_package.push_to_buffer('uint32_t', int(counter_max))
            await MC.I().write_mc_data_package(mc_data_package)
            sleep(0.1)
            return await self.check_for_ack(writer=(writer if respond else None))

    async def set_sampling_rate(self, sampling_rate:int, writer, respond=True):
        if sampling_rate > BackendSettings.mc_internal_clock_rate:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            # get counter_max and prescaler for desired sampling rate
            fraction = Fraction.from_float(sampling_rate / BackendSettings.mc_internal_clock_rate)
            fraction = fraction.limit_denominator(BackendSettings.mc_max_prescaler)
            prescaler = fraction.denominator
            counter_max = fraction.numerator

            self.prescaler = prescaler
            self.counter_max = counter_max

            logging.debug('Backend: set_sampling_rate')
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 19) # method_identifier
            mc_data_package.push_to_buffer('uint32_t', prescaler)
            mc_data_package.push_to_buffer('uint32_t', counter_max)
            await MC.I().write_mc_data_package(mc_data_package)
            return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_one_chunks(self, chunks, writer, respond=True):
        if len(chunks) != self.chunks_one_size:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            self.chunks_one = chunks

            logging.debug('Backend: set ch one chunks')
            # send buffer in chunks of MCDataPackage.MAX_NBR_BYTES
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 20) # method_identifier
            for f in chunks:
                mc_data_package.push_to_buffer('uint32_t', f)
            await MC.I().write_mc_data_package(mc_data_package)
            return await self.check_for_ack(writer=(writer if respond else None))

    async def set_ch_two_chunks(self, chunks, writer, respond=True):
        if len(chunks) != self.chunks_two_size:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            self.chunks_two = chunks

            logging.debug('Backend: set ch two chunks')
            # send buffer in chunks of MCDataPackage.MAX_NBR_BYTES
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 21) # method_identifier
            for f in chunks:
                mc_data_package.push_to_buffer('uint32_t', f)
            await MC.I().write_mc_data_package(mc_data_package)
            return await self.check_for_ack(writer=(writer if respond else None))

    async def check_for_ack(self, writer=None):
        ack =  await MC.I().read_ack()
        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        for key in self.__dict__:
            if key not in config:
                config[key] = self.__dict__[key]
        return config

    async def launch_from_config(self, config_dict):
        try:
            pass
            # await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
            #                     config_dict['out_range_max'], config_dict['useTTL'], config_dict['locked'], None)

            # await  super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'AWGModule: canot launch_from_config: {ex}')
            raise ex


