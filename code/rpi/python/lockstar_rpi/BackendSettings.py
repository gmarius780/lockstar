from os.path import join, dirname
class BackendSettings:
    """Hardcoded settings for the backend"""
    backend_ip = '0.0.0.0'
    backend_port = 10780
    # backend_port = 10780
    read_buffer_limit_bytes = 50 * 10 ** 6
    current_module_config_file = './current_module.config'
    mc_communication_speed_Hz = 1000000
    mc_gpio_input_channel = 8
    debug_mode = False
    mc_write_buffer_size = 4096
    mc_internal_clock_rate = 90e6
    mc_sampling_clock_rate = 45e6 #currently TIM2 is used for sampling, which is connected to the APB1 clock domain, which runs at 45MHz
    mc_max_counter = 65536
    elf_directory = join(dirname(__file__), 'Modules', 'mc_images')
    #max nbr of floats per channel that can be recorded, must be equal to the value
    #in Scope.h
    scope_max_buffer_length_nbr_of_floats = 10000 
    scope_max_sampling_rate = 10000