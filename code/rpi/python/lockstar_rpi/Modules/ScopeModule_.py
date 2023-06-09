import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_rpi.Helpers.SamplingRate import SamplingRate

from math import floor, ceil
from time import perf_counter


class ScopeModule_(IOModule_):
    """This Module should not be used directly by the client (hence the _ at the end of the name).
    It contains functionality which is shared by most modules (e.g. linearization of the DAC)
    """

    def __init__(self) -> None:
        self.scope_max_buffer_length_nbr_of_floats = None
        super().__init__()

        self.scope_nbr_samples_in_one = 0
        self.scope_nbr_samples_in_two = 0
        self.scope_nbr_samples_out_one = 0
        self.scope_nbr_samples_out_two = 0
        self.scope_adc_active_mode = False
        self.scope_double_buffer_mode = False
        self.scope_sampling_rate = 0
        self.scope_setup = False
        self.scope_enabled = False

        if self.scope_max_buffer_length_nbr_of_floats is None:
            logging.error(f'scope_max_buffer_length_nbr_of_floats not specified in constructor of RPI-Module. Each module must specify this and it must \
                          correspond to the value set in the compiler directive SCOPE_BUFFER_LENGTH on the microcontroller')

    
     # ==== START: client methods 
    async def setup_scope(self, sampling_rate: int, nbr_samples_in_one: int, nbr_samples_in_two: int, \
        nbr_samples_out_one: int, nbr_samples_out_two: int, adc_active_mode: bool, double_buffer_mode: bool, writer, respond=True):
        """Sets up the scope such that recorded data can be querried by calling scope_get_data. Data can be recorded from
        all analog inputs and outputs by putting the appropriate sample_<in/out>_<one_two> to true. The number of samples
        per channel can be set by <buffer_length> and the sampling rate by <sampling_rate>. If adc_active_mode==True, the scope
        calls the adc->start_conversion function itself according to the sampling rate. Otherwise, the scope simply returns
        the last recorded value, this uses less resources and can be useful if one wants to 'see what the workloop sees'.

        Args:
            sampling_rate (int): scope sampling rate (positive integer (Hz))
            nbr_samples_in_one (int): nbr of samples to record for input one
            nbr_samples_in_two (int): nbr of samples to record for input two
            nbr_samples_out_one (int): nbr of samples to record for output one
            nbr_samples_out_two (int): nbr of samples to record for output two
            adc_active_mode (bool): whether or not the scope should call adc->start_conversion itself
            double_buffer_mode (bool): two buffers will be created such that one can be read out while the other is being written to
        """

        logging.debug('ScopeModule_: setup_scope')
        total_nbr_samples = nbr_samples_in_one + nbr_samples_in_two + nbr_samples_out_one + nbr_samples_out_two
        sub_zero_samples = nbr_samples_in_one < 0 or nbr_samples_in_two < 0 or nbr_samples_out_one < 0 or nbr_samples_out_two < 0
        
        if self.scope_max_buffer_length_nbr_of_floats is None or sampling_rate <= 0 or sub_zero_samples or \
            total_nbr_samples > self.scope_max_buffer_length_nbr_of_floats or \
            total_nbr_samples <= 0 or sampling_rate > BackendSettings.scope_max_sampling_rate:
            logging.error('ScopeModule_ - setup_scope: invalid parameters')
            if writer is not None and respond:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        else:
            prescaler, counter_max = SamplingRate.calculate_prescaler_counter(sampling_rate)

            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 100) # method_identifier

            mc_data_package.push_to_buffer('uint32_t', prescaler)
            mc_data_package.push_to_buffer('uint32_t', counter_max)

            mc_data_package.push_to_buffer('uint32_t', nbr_samples_in_one)
            mc_data_package.push_to_buffer('uint32_t', nbr_samples_in_two)
            mc_data_package.push_to_buffer('uint32_t', nbr_samples_out_one)
            mc_data_package.push_to_buffer('uint32_t', nbr_samples_out_two)

            mc_data_package.push_to_buffer('bool', adc_active_mode)
            mc_data_package.push_to_buffer('bool', double_buffer_mode)

            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.scope_sampling_rate = sampling_rate
                self.scope_nbr_samples_in_one = nbr_samples_in_one
                self.scope_nbr_samples_in_two = nbr_samples_in_two
                self.scope_nbr_samples_out_one = nbr_samples_out_one
                self.scope_nbr_samples_out_two = nbr_samples_out_two
                self.scope_adc_active_mode = adc_active_mode
                self.scope_double_buffer_mode = double_buffer_mode
                self.scope_setup = True
            return result

    async def set_scope_sampling_rate(self, sampling_rate: int, writer, respond=True):
        """Sets the sampling rate of the scope  

        Args:
            sampling_rate (int): scope sampling rate (positive integer (Hz))
        """
        logging.debug('ScopeModule_: set_scope_sampling_rate')
        if sampling_rate <= 0 or sampling_rate > BackendSettings.scope_max_sampling_rate:
            logging.error('ScopeModule_ - set_scope_sampling_rate: invalid parameters')
            if writer is not None and respond:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
        else:
            prescaler, counter_max = SamplingRate.calculate_prescaler_counter(sampling_rate)

            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 104) # method_identifier

            mc_data_package.push_to_buffer('uint32_t', prescaler)
            mc_data_package.push_to_buffer('uint32_t', counter_max)


            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.scope_sampling_rate = sampling_rate
            return result

    async def enable_scope(self, writer, respond=True):
        """if scope is setup, the sampling timer is enabled
        """
        logging.debug('ScopeModule_: enable_scope')
        if self.scope_setup:
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 101) # method_identifier
            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.scope_enabled = True
            return result
        else:
            logging.error('ScopeModule_ - enable_scope: scope is not setup')
            if writer is not None and respond:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False


    async def disable_scope(self, writer, respond=True):
        """if scope is setup, the sampling timer is disable
        """
        logging.debug('ScopeModule_: disable_scope')
        if self.scope_setup:
            mc_data_package = MCDataPackage()
            mc_data_package.push_to_buffer('uint32_t', 102) # method_identifier
            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.scope_enabled = False
            return result
        else:
            logging.error('ScopeModule_ - disable_scope: scope is not setup')
            if writer is not None and respond:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False

    async def get_scope_data(self, writer, respond=True):
        """returns the data sampled by the scope
        """
        METHOD_IDENTIFIER = 103
        logging.debug('ScopeModule_: get_scope_data')
        if self.scope_setup and writer is not None:
            t_start = perf_counter()


            #prepare output dict
            nbr_of_samples_per_channel = [self.scope_nbr_samples_in_one, self.scope_nbr_samples_in_two, 
                                          self.scope_nbr_samples_out_one, self.scope_nbr_samples_out_two]
            
            total_nbr_samples = sum(nbr_of_samples_per_channel)

            sample_dict_keys = ['in_one', 'in_two', 'out_one', 'out_two']

            buffer = [0]*total_nbr_samples


            # max nbr of samples that can be downloaded from the MC per request
            max_package_size = floor((MCDataPackage.MAX_NBR_BYTES_READ - 100)/4)
            nbr_of_packages = ceil(total_nbr_samples / max_package_size)
            nbr_of_full_packages = total_nbr_samples // max_package_size

            buffer_offset = 0
            logging.debug(f'get_scope_data: expecting {nbr_of_packages} packages.')
            for package_number in range(nbr_of_full_packages):
                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
                mc_data_package.push_to_buffer('uint32_t',buffer_offset)
                mc_data_package.push_to_buffer('uint32_t',max_package_size)
                await MC.I().write_mc_data_package(mc_data_package)
                
                datatype_list = ['float']*max_package_size
                response = await MC.I().read_mc_data_package(datatype_list)
                if response == False:
                    logging.error(f'ScopeModule.get_scope_data - read package of length: {max_package_size} failed!')
                    if writer is not None and respond:
                        writer.write(BackendResponse.NACK().to_bytes())
                        await writer.drain()
                    return False
                else:
                    response_length, response_list = response
                    
                    i_start_trace = package_number*max_package_size
                    i_end_trace = i_start_trace + max_package_size

                    #store results in corresponding dict entry
                    buffer[i_start_trace:i_end_trace] = response_list[i_start_trace:i_end_trace]
        
                    buffer_offset += max_package_size
                    logging.debug(f"get_scope_data: received package number {package_number+1}.")

            if nbr_of_full_packages < nbr_of_packages:
                remaining_package_size = total_nbr_samples%max_package_size

                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
                mc_data_package.push_to_buffer('uint32_t',buffer_offset)
                mc_data_package.push_to_buffer('uint32_t',remaining_package_size)
                await MC.I().write_mc_data_package(mc_data_package)

                datatype_list = ['float']*remaining_package_size

                response = await MC.I().read_mc_data_package(datatype_list)
                if response == False:
                    logging.error(f'ScopeModule.get_scope_data - read package of length: {remaining_package_size} failed!')
                    if writer is not None and respond:
                        writer.write(BackendResponse.NACK().to_bytes())
                        await writer.drain()
                    return False
                else: 
                    response_length, response_list = response

                    buffer[-remaining_package_size:] = response_list[0:remaining_package_size]
            
            #Write traces
            scope_traces = {}
            i_buffer = 0
            for nbr_samples, sample_dict_key in zip(nbr_of_samples_per_channel, sample_dict_keys):
                    if nbr_samples > 0:
                        scope_traces[sample_dict_key] = buffer[i_buffer:i_buffer+nbr_samples]
                        i_buffer += nbr_samples
                    else:
                        scope_traces[sample_dict_key] = []

            logging.debug(f'MC-communication time: {perf_counter() - t_start:.1f}s')
            t_start = perf_counter()
            br = BackendResponse(scope_traces)
            writer.write(br.to_bytes())
            await writer.drain()
            logging.debug(f'client communication time: {perf_counter() - t_start:.1f}s')
            logging.debug('ScopeModule: got scope successful!')
            return True


        else:
            logging.error('ScopeModule_ - get_scope_data: scope is not setup')
            if writer is not None and respond:
                writer.write(BackendResponse.NACK().to_bytes())
                await writer.drain()
            return False
     # ==== END: client methods


    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        return config

    async def launch_from_config(self, config_dict):
        try:
            await super().launch_from_config(config_dict)
            
            #setup scope if neccessary
            if 'scope_setup' in config_dict.keys() and config_dict['scope_setup'] == True:
                
                if 'scope_nbr_samples_in_one' in config_dict.keys() and \
                    'scope_nbr_samples_in_two' in config_dict.keys() and 'scope_nbr_samples_out_one' in config_dict.keys() and 'scope_nbr_samples_out_two' in config_dict.keys() and \
                        'scope_adc_active_mode' in config_dict.keys() and 'scope_sampling_rate' in config_dict.keys() and 'scope_enabled' in config_dict.keys():
                    retry_counter = 10
                    success = False
                    while retry_counter > 0 and not success:
                        success = await self.setup_scope(config_dict['scope_sampling_rate'], config_dict['scope_nbr_samples_in_one'], config_dict['scope_nbr_samples_in_two'], config_dict['scope_nbr_samples_out_one'],
                                                        config_dict['scope_nbr_samples_out_two'], config_dict['scope_adc_active_mode'], config_dict['double_buffer_mode'], None, respond=False)
                        retry_counter -= 1
                    if not success:
                        raise Exception('ScopeModule_ - canot launch from config_dict: cannot setup scope')
                    else:
                        if config_dict['scope_enabled'] == True:
                            retry_counter = 10
                            success = False
                            while retry_counter > 0 and not success:
                                success = await self.enable_scope(None, respond=False)
                                retry_counter -= 1
                            if not success:
                                raise Exception('ScopeModule_ - canot launch from config_dict: cannot setup scope')

        except Exception as ex:
            logging.error(f'ScopeModule_: canot launch_from_config: {ex}')
            raise ex
