from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_rpi.Modules.ScopeModule_ import ScopeModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
from math import floor
from time import perf_counter

from lockstar_rpi.Helpers.SamplingRate import SamplingRate

class BufferBaseModule_(ScopeModule_):
    """Base class for AWGModule and AWGPIDModule. Implements the functionality to have two buffers which the user can use to 
    store arbitrary waveforms. Additionally it allows to split the buffers into chunks, which are output individually.
    """
    BUFFER_LIMIT_kBYTES = 160
    MAX_NBR_OF_CHUNKS = 100


    def __init__(self) -> None:
        super().__init__()
        self.is_output_ttl = False
        self.buffer_one = []
        self.buffer_two = []
        self.chunks_one = []
        self.chunks_two = []
        self.buffer_one_size = 0
        self.buffer_two_size = 0
        self.chunks_one_size = 0
        self.chunks_two_size = 0
        self.sampling_rate = 0

    # ==== START: client methods 
    async def initialize(self, writer):
        pass

    async def output_on(self, writer, respond=True):
        """Output the next chunk on both outputs. If the last chunk has been reached,
        it starts again with the first one.
        """
        logging.debug('Backend: output_on')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.is_output_ttl = False
        return result
    
    async def output_off(self, writer, respond=True):
        """Stop output. All pointers are reset to point to the first chunk"""
        logging.debug('Backend: output_off')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.is_output_ttl = False
        return result

    async def output_ttl(self, writer, respond=True):
        """Start listening to the digital input for trigger signals. Once a trigger is received,
        it will play the next chunk
        """
        logging.debug('Backend: output_ttl')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        t_start = perf_counter()
        await MC.I().write_mc_data_package(mc_data_package)
        logging.info(f'---------time to write mc_data_package: {perf_counter() - t_start} s')
        t_start = perf_counter()
        result = await self.check_for_ack(writer=(writer if respond else None))
        logging.info(f'---------time to write check_for_ack: {perf_counter() - t_start} s')
        if result:
            self.is_output_ttl = True
        return result

    async def set_ch_one_buffer(self, buffer, writer, respond=True):
        """upload buffer for channel one waveforms (buffer-length has to be defined beforhand using initialize_buffers)"""
        return await self.set_ch_buffer(buffer, writer, respond, buffer_one=True)

    async def set_ch_two_buffer(self, buffer, writer, respond=True):
        """upload buffer for channel one waveforms (buffer-length has to be defined beforhand using initialize_buffers)"""
        return await self.set_ch_buffer(buffer, writer, respond, buffer_one=False)

    async def set_ch_buffer(self, buffer, writer, respond, buffer_one=True):
        """helper method to read in buffer from client. Buffer sent to the MC in packets of floor(MCDataPackage.MAX_NBR_BYTES - 100)/4 floats"""
        if len(buffer) > (self.buffer_one_size if buffer_one else self.buffer_two_size):
            logging.error(f'set_ch_{"one" if buffer_one else "two"}_buffer - buffer too large: {len(buffer)}')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:

            logging.debug(f'Backend: set ch {"one" if buffer_one else "two"} buffer')
            # send buffer in packets of floor(MCDataPackage.MAX_NBR_BYTES - 100)/4 floats
            number_of_floats_per_package = floor((MCDataPackage.MAX_NBR_BYTES_TO_MC - 100)/4)
            for i in range(0, len(buffer), number_of_floats_per_package):
                
                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t', (16 if buffer_one else 17)) # method_identifier
                if i == 0:
                    mc_data_package.push_to_buffer('bool', False) #overwrite buffer
                else:
                    mc_data_package.push_to_buffer('bool', True) #append to buffer
                    
                nbr_values_to_read = number_of_floats_per_package
                if i + number_of_floats_per_package > len(buffer):
                    nbr_values_to_read = len(buffer) - i
                
                mc_data_package.push_to_buffer('uint32_t', nbr_values_to_read)
                for f in buffer[i:i+nbr_values_to_read]:
                    mc_data_package.push_to_buffer('float', f)
                logging.debug(f'send {nbr_values_to_read} floats')
                await MC.I().write_mc_data_package(mc_data_package)
                #wait for acknowledgment of reception by MC
                ack = await self.check_for_ack(writer=None)
                if not ack:
                    logging.error(f'set ch {"one" if buffer_one else "two"} buffer: could not send packet!!')
                    if writer is not None:
                        writer.write(BackendResponse.NACK().to_bytes())
                        await writer.drain()
                        return False
            
            if buffer_one:
                self.buffer_one = buffer.copy()
            else:
                self.buffer_two = buffer.copy()

            if writer is not None:
                writer.write(BackendResponse.ACK().to_bytes())
                await writer.drain()
            return True

    async def initialize_buffers(self, buffer_one_size: int, buffer_two_size: int, chunks_one_size: int, 
                                chunks_two_size: int, sampling_rate:int, writer, respond=True):
        """sets the sizes of the buffer_one and buffer two, numbers of chunks for buffer_one 
        and buffer_two as well as the sampling rate

        
        :param buffer_one_size (int): number of floats for channel one buffer
        :param buffer_two_size (int): number of floats for channel two buffer
        :param chunks_one_size (int): number of chunks in channel one buffer
        :param chunks_two_size (int): number of chunks in channel two buffer
        :param sampling_rate (int): sampling rate in Hz
        :param writer (_type_): _description_
        :param respond (bool, optional): _description_. Defaults to True.
        """
        
        if buffer_one_size + buffer_two_size > BufferBaseModule_.BUFFER_LIMIT_kBYTES*250:
            logging.warning(f'initialize_buffers wrong arguments: buffer_sizes: ({buffer_one_size},{buffer_two_size})')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        elif chunks_one_size + chunks_two_size > BufferBaseModule_.MAX_NBR_OF_CHUNKS:
            logging.warning(f'initialize_buffers wrong arguments: chunksize: ({chunks_one_size},{chunks_two_size})')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        elif chunks_one_size <= 0 or buffer_one_size <= 0 or chunks_two_size <= 0 or buffer_two_size <= 0:
            logging.warning(f'Both chunk and buffer sizes must be at least 1')
            if writer is not None:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        else:
            # fraction = Fraction.from_float(sampling_rate / BackendSettings.mc_internal_clock_rate)
            # fraction = fraction.limit_denominator(BackendSettings.mc_max_counter)
            # counter_max = fraction.denominator
            # prescaler = fraction.numerator
            self.prescaler, self.counter_max = SamplingRate.calculate_prescaler_counter(sampling_rate)

            logging.info(f'initialize: sampling rate: {sampling_rate}, prescaler: {self.prescaler}, counter_max: {self.counter_max}')

            logging.debug('Backend: initialize_buffers')
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 18) # method_identifier
            mc_data_package.push_to_buffer('uint32_t', buffer_one_size)
            mc_data_package.push_to_buffer('uint32_t', buffer_two_size)
            mc_data_package.push_to_buffer('uint32_t', chunks_one_size)
            mc_data_package.push_to_buffer('uint32_t', chunks_two_size)
            mc_data_package.push_to_buffer('uint32_t', self.prescaler)
            mc_data_package.push_to_buffer('uint32_t', self.counter_max)
            await MC.I().write_mc_data_package(mc_data_package)
            # sleep(0.1)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.buffer_one_size = buffer_one_size
                self.buffer_two_size = buffer_two_size
                self.chunks_one_size = chunks_one_size
                self.chunks_two_size = chunks_two_size
                self.sampling_rate = sampling_rate
            return result

    

    async def set_sampling_rate(self, sampling_rate:int, writer, respond=True):
        """ Set sampling rate in Hz

        Args:
        :param    sampling_rate (int): sampling rate in Hz

        """
        if sampling_rate > BackendSettings.mc_internal_clock_rate:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            # get counter_max and prescaler for desired sampling rate
            # fraction = Fraction.from_float(BackendSettings.mc_internal_clock_rate/sampling_rate)
            # fraction = fraction.limit_denominator(BackendSettings.mc_max_counter)
            # counter_max = fraction.denominator
            # prescaler = fraction.numerator

            self.prescaler, self.counter_max = SamplingRate.calculate_prescaler_counter(sampling_rate)

            logging.debug('Backend: set_sampling_rate')
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 19) # method_identifier
            mc_data_package.push_to_buffer('uint32_t', self.prescaler)
            mc_data_package.push_to_buffer('uint32_t', self.counter_max)
            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.sampling_rate = sampling_rate
            return result

    async def set_ch_one_chunks(self, chunks, writer, respond=True):
        """Upload a list of buffer-indices which define the 'chunks': A chunk is a piece of the buffer which the lockstar outputs
        once it receives a trigger or an output_on(). A chunk is defined by the last buffer-index which still bellongs to this chunk.
        example: buffer-length: 10'000. To split this into 3 pieces of 5000, 2000, 3000: chunks=[4999, 6999, 9999]
        
        :param chunks (list of integers): corresponding to the final indices of the chunks
        
        """
        if len(chunks) != self.chunks_one_size:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            

            logging.debug('Backend: set ch one chunks')
            # send buffer in chunks of MCDataPackage.MAX_NBR_BYTES
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 20) # method_identifier
            for f in chunks:
                mc_data_package.push_to_buffer('uint32_t', f)
            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.chunks_one = chunks
            return result

    async def set_ch_two_chunks(self, chunks, writer, respond=True):
        """Upload a list of buffer-indices which define the 'chunks': A chunk is a piece of the buffer which the lockstar outputs
        once it receives a trigger or an output_on(). A chunk is defined by the last buffer-index which still bellongs to this chunk.
        example: buffer-length: 10'000. To split this into 3 pieces of 5000, 2000, 3000: chunks=[4999, 6999, 9999]
        
        :param chunks (list of integers): corresponding to the final indices of the chunks
        
        """
        if len(chunks) != self.chunks_two_size:
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            logging.debug('Backend: set ch two chunks')
            # send buffer in chunks of MCDataPackage.MAX_NBR_BYTES
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 21) # method_identifier
            for f in chunks:
                mc_data_package.push_to_buffer('uint32_t', f)
            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.chunks_two = chunks
            return result

    async def get_ch_one_chunks(self, writer):
        br = BackendResponse(self.chunks_one)
        writer.write(br.to_bytes())
        await writer.drain()
        return True
    
    async def get_ch_two_chunks(self, writer):
        br = BackendResponse(self.chunks_two)
        writer.write(br.to_bytes())
        await writer.drain()
        return True
    
    async def get_ch_one_buffer(self, writer):
        br = BackendResponse(self.buffer_one)
        writer.write(br.to_bytes())
        await writer.drain()
        return True

    async def get_ch_two_buffer(self, writer):
        br = BackendResponse(self.buffer_two)
        writer.write(br.to_bytes())
        await writer.drain()
        return True
    
    async def get_sampling_rate(self, writer):
        br = BackendResponse(self.sampling_rate)
        writer.write(br.to_bytes())
        await writer.drain()
        return True
    

    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        return config

    async def launch_from_config(self, config_dict):
        try:
            await super().launch_from_config(config_dict)
            
            # set buffers etc
            if 'buffer_one_size' in config_dict.keys() and 'chunks_one_size' in config_dict.keys() and 'sampling_rate' in config_dict.keys() and \
                'buffer_two_size' in config_dict.keys() and 'chunks_two_size' in config_dict.keys():
                retry_counter = 10
                success = False
                while retry_counter > 0 and not success:
                    success = await self.initialize_buffers(config_dict['buffer_one_size'], config_dict['buffer_two_size'],
                                                                config_dict['chunks_one_size'], config_dict['chunks_two_size'], config_dict['sampling_rate'],
                                                                None, respond=False)
                    retry_counter -= 1
                if not success:
                    raise Exception('BufferBaseModule - canot launch from exception: cannot initialize buffer')
                else:
                    if 'buffer_one' in config_dict.keys() and 'chunks_one' in config_dict.keys():
                        retry_counter = 10
                        success = False
                        while retry_counter > 0 and not success:
                            success = await self.set_ch_one_buffer(config_dict['buffer_one'], None, respond=False)
                            retry_counter -= 1
                        
                        if not success:
                            raise Exception('BufferBaseModule - canot launch from exception: cannot set buffer_one')
                        else:
                            retry_counter = 10
                            success = False
                            while retry_counter > 0 and not success:
                                success = await self.set_ch_one_chunks(config_dict['chunks_one'], None, respond=True)
                                retry_counter -= 1
                        if not success:
                            raise Exception('BufferBaseModule - canot launch from exception: cannot set chunks_one')
                    
                    if 'buffer_two' in config_dict.keys() and 'chunks_two' in config_dict.keys():
                        retry_counter = 10
                        success = False
                        while retry_counter > 0 and not success:
                            success = await self.set_ch_two_buffer(config_dict['buffer_two'], None, respond=False)
                            retry_counter -= 1
                        
                        if not success:
                            raise Exception('BufferBaseModule - canot launch from exception: cannot set buffer_two')
                        else:
                            retry_counter = 10
                            success = False
                            while retry_counter > 0 and not success:
                                success = await self.set_ch_two_chunks(config_dict['chunks_two'], None, respond=True)
                                retry_counter -= 1
                        if not success:
                            raise Exception('BufferBaseModule - canot launch from exception: cannot set chunks_two')

        except Exception as ex:
            logging.error(f'AWGModule: canot launch_from_config: {ex}')
            raise ex


