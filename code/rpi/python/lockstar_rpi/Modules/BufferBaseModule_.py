from time import sleep
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
import numpy as np
from math import ceil, floor

class BufferBaseModule_(IOModule_):
    """Base class for AWGModule and AWGPIDModule. Implements the functionality to have two buffers which the user can use to 
    store arbitrary waveforms. Additionally it allows to split the buffers into chunks, which are output individually.
    """
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
        """Output the next chunk on both outputs. If the last chunk has been reached,
        it starts again with the first one.
        """
        logging.debug('Backend: output_on')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))
    
    async def output_off(self, writer, respond=True):
        """Stop output. All pointers are reset to point to the first chunk"""
        logging.debug('Backend: output_off')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def output_ttl(self, writer, respond=True):
        """Start listening to the digital input for trigger signals. Once a trigger is received,
        it will play the next chunk
        """
        logging.debug('Backend: output_ttl')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

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
            if buffer_one:
                self.buffer_one = buffer
            else:
                self.buffer_two = buffer

            logging.debug(f'Backend: set ch {"one" if buffer_one else "two"} buffer')
            # send buffer in packets of floor(MCDataPackage.MAX_NBR_BYTES - 100)/4 floats
            number_of_floats_per_package = floor((MCDataPackage.MAX_NBR_BYTES - 100)/4)
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
                logging.debug(f'send packet of length: {nbr_values_to_read}')
                await MC.I().write_mc_data_package(mc_data_package)
                #wait for acknowledgment of reception by MC
                ack = await self.check_for_ack(writer=None)
                if not ack:
                    logging.error(f'set ch {"one" if buffer_one else "two"} buffer: could not send packet!!')
                    if writer is not None:
                        writer.write(BackendResponse.NACK().to_bytes())
                        await writer.drain()
                        return False
                
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
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        elif chunks_one_size + chunks_two_size > BufferBaseModule_.MAX_NBR_OF_CHUNKS:
            logging.warning(f'initialize_buffers wrong arguments: chunksize: ({chunks_one_size},{chunks_two_size})')
            writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
            return False
        else:
            # fraction = Fraction.from_float(sampling_rate / BackendSettings.mc_internal_clock_rate)
            # fraction = fraction.limit_denominator(BackendSettings.mc_max_counter)
            # counter_max = fraction.denominator
            # prescaler = fraction.numerator
            self.prescaler, self.counter_max = BufferBaseModule_.calculate_prescaler_counter(sampling_rate)

            logging.info(f'initialize: sampling rate: {sampling_rate}, prescaler: {self.prescaler}, counter_max: {self.counter_max}')

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
            mc_data_package.push_to_buffer('uint32_t', self.prescaler)
            mc_data_package.push_to_buffer('uint32_t', self.counter_max)
            await MC.I().write_mc_data_package(mc_data_package)
            sleep(0.1)
            return await self.check_for_ack(writer=(writer if respond else None))

    @staticmethod
    def calculate_prescaler_counter(sampling_rate):
        """Calculate prescaler and counter for a given sampling_rate. The MC realizes a sampling rate by scaling down 
        the internal clock_frequency (BackendSettings.mc_internal_clock_rate) with the prescaler and then counting up to
        <counter> --> sampling_rate = internal_clock_rate / prescaler / counter"""
        rate = BackendSettings.mc_internal_clock_rate / sampling_rate
        possible_prescalers = np.flip(np.arange(ceil(rate/BackendSettings.mc_max_counter), BackendSettings.mc_max_counter))
        possible_counters = rate/possible_prescalers
        best_counter = int(possible_counters[np.abs(possible_counters - possible_counters.astype(int)).argmin()])
        best_prescaler = int(rate/best_counter)
        return best_prescaler , best_counter

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

            self.prescaler, self.counter_max = BufferBaseModule_.calculate_prescaler_counter(sampling_rate)

            logging.debug('Backend: set_sampling_rate')
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 19) # method_identifier
            mc_data_package.push_to_buffer('uint32_t', self.prescaler)
            mc_data_package.push_to_buffer('uint32_t', self.counter_max)
            await MC.I().write_mc_data_package(mc_data_package)
            return await self.check_for_ack(writer=(writer if respond else None))

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
        """Waits for ACK/NACK from the MC and responds accordingly to the client"""
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


