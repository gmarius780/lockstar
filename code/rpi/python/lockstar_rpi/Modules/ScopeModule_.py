import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
from lockstar_rpi.BackendSettings import BackendSettings
from lockstar_rpi.Helpers.SamplingRate import SamplingRate

from math import floor, ceil


class ScopeModule_(IOModule_):
    """This Module should not be used directly by the client (hence the _ at the end of the name).
    It contains functionality which is shared by most modules (e.g. linearization of the DAC)
    """

    def __init__(self) -> None:
        super().__init__()

        self.scope_buffer_length = 0
        self.scope_sample_in_one = False
        self.scope_sample_in_two = False
        self.scope_sample_out_one = False
        self.scope_sample_out_two = False
        self.scope_adc_active_mode = False
        self.scope_sampling_rate = 0
        self.scope_setup = False
        self.scope_enabled = False

    
     # ==== START: client methods 
    async def setup_scope(self, sampling_rate: int, sample_in_one: bool, sample_in_two: bool, \
        sample_out_one: bool, sample_out_two: bool, buffer_length: int, adc_active_mode: bool, writer, respond=True):
        """Sets up the scope such that recorded data can be querried by calling scope_get_data. Data can be recorded from
        all analog inputs and outputs by putting the appropriate sample_<in/out>_<one_two> to true. The number of samples
        per channel can be set by <buffer_length> and the sampling rate by <sampling_rate>. If adc_active_mode==True, the scope
        calls the adc->start_conversion function itself according to the sampling rate. Otherwise, the scope simply returns
        the last recorded value, this uses less resources and can be useful if one wants to 'see what the workloop sees'.

        Args:
            sampling_rate (int): scope sampling rate (positive integer (Hz))
            sample_in_one (bool): whether or not to record the corresponding channel
            sample_in_two (bool): whether or not to record the corresponding channel
            sample_out_one (bool): whether or not to record the corresponding channel
            sample_out_two (bool): whether or not to record the corresponding channel
            buffer_length (int): number of floats per activated channel to be recorded
            adc_active_mode (bool): whether or not the scope should call adc->start_conversion itself
        """
        logging.debug('ScopeModule_: setup_scope')
        if sampling_rate <= 0 or buffer_length <= 0 or buffer_length > BackendSettings.scope_max_buffer_length_nbr_of_floats or \
            (not sample_in_one and not sample_in_two and not sample_out_one and not sample_out_two) or \
                sampling_rate > BackendSettings.scope_max_sampling_rate:
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

            mc_data_package.push_to_buffer('bool', sample_in_one)
            mc_data_package.push_to_buffer('bool', sample_in_two)
            mc_data_package.push_to_buffer('bool', sample_out_one)
            mc_data_package.push_to_buffer('bool', sample_out_two)

            mc_data_package.push_to_buffer('uint32_t', buffer_length)
            mc_data_package.push_to_buffer('bool', adc_active_mode)

            await MC.I().write_mc_data_package(mc_data_package)
            result = await self.check_for_ack(writer=(writer if respond else None))
            if result:
                self.scope_sampling_rate = sampling_rate
                self.scope_sample_in_one = sample_in_one
                self.scope_sample_in_two = sample_in_two
                self.scope_sample_out_one = sample_out_one
                self.scope_sample_out_two = sample_out_two
                self.scope_buffer_length = buffer_length
                self.scope_adc_active_mode = adc_active_mode
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
            
            nbr_of_recorded_channels = 0
            if self.scope_sample_in_one: nbr_of_recorded_channels += 1
            if self.scope_sample_in_two: nbr_of_recorded_channels += 1
            if self.scope_sample_out_one: nbr_of_recorded_channels += 1
            if self.scope_sample_out_two: nbr_of_recorded_channels += 1

            #prepare output dict
            sample_bools = [self.scope_sample_in_one, self.scope_sample_in_two, self.scope_sample_out_one, self.scope_sample_out_two]
            sample_dict_keys = ['in_one', 'in_two', 'out_one', 'out_two']

            scope_traces = {}

            for sample_bool, sample_dict_key in zip(sample_bools, sample_dict_keys):
                if sample_bool:
                    scope_traces[sample_dict_key] = [0]*self.scope_buffer_length
                else:
                    scope_traces[sample_dict_key] = []

            # max nbr of samples PER CHANNEL that can be downloaded from the MC per request
            max_package_size = floor(floor((MCDataPackage.MAX_NBR_BYTES - 100)/4)/nbr_of_recorded_channels)
            nbr_of_packages = ceil(self.scope_buffer_length / max_package_size)
            nbr_of_full_packages = self.scope_buffer_length // max_package_size

            buffer_offset = 0
            logging.debug(f'get_scope_data: expecting {nbr_of_packages} packages.')
            for package_number in range(nbr_of_full_packages):
                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
                mc_data_package.push_to_buffer('uint32_t',buffer_offset)
                mc_data_package.push_to_buffer('uint32_t',max_package_size)
                await MC.I().write_mc_data_package(mc_data_package)
                
                datatype_list = ['float']*max_package_size*nbr_of_recorded_channels
                response_length, response_list = await MC.I().read_mc_data_package(datatype_list)
                
                i_start_trace = package_number*max_package_size
                i_end_trace = i_start_trace + max_package_size
                i_start_package = 0
                i_end_package = max_package_size

                #store results in corresponding dict entry
                for sample_bool, sample_dict_key in zip(sample_bools, sample_dict_keys):
                    if sample_bool:
                        scope_traces[sample_dict_key][i_start_trace:i_end_trace] = response_list[i_start_package:i_end_package]
                        i_start_package += max_package_size
                        i_end_package += max_package_size
    
                buffer_offset += max_package_size
                logging.debug(f"get_scope_data: received package number {package_number+1}.")

            if nbr_of_full_packages < nbr_of_packages:
                remaining_package_size = self.scope_buffer_length%max_package_size

                mc_data_package = MCDataPackage()
                mc_data_package.push_to_buffer('uint32_t',METHOD_IDENTIFIER)
                mc_data_package.push_to_buffer('uint32_t',buffer_offset)
                mc_data_package.push_to_buffer('uint32_t',remaining_package_size)
                await MC.I().write_mc_data_package(mc_data_package)

                datatype_list = ['float']*remaining_package_size*nbr_of_recorded_channels

                response_length, response_list = await MC.I().read_mc_data_package(datatype_list)

                i_start_package = 0
                i_end_package = remaining_package_size
                #store results in corresponding dict entry
                for sample_bool, sample_dict_key in zip(sample_bools, sample_dict_keys):
                    if sample_bool:
                        scope_traces[sample_dict_key][-remaining_package_size:] = response_list[i_start_package:i_end_package]
                        i_start_package += remaining_package_size
                        i_end_package += remaining_package_size
            
            br = BackendResponse(scope_traces)
            writer.write(br.to_bytes())
            await writer.drain()
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
                
                if 'scope_buffer_length' in config_dict.keys() and 'scope_sample_in_one' in config_dict.keys() and \
                    'scope_sample_in_two' in config_dict.keys() and 'scope_sample_out_one' in config_dict.keys() and 'scope_sample_out_two' in config_dict.keys() and \
                        'scope_adc_active_mode' in config_dict.keys() and 'scope_sampling_rate' in config_dict.keys() and 'scope_enabled' in config_dict.keys():
                    retry_counter = 10
                    success = False
                    while retry_counter > 0 and not success:
                        success = await self.setup_scope(config_dict['scope_sampling_rate'], config_dict['scope_sample_in_one'], config_dict['scope_sample_in_two'], config_dict['scope_sample_out_one'],
                                                        config_dict['scope_sample_out_two'], config_dict['scope_buffer_length'], config_dict['scope_adc_active_mode'], None, respond=False)
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
