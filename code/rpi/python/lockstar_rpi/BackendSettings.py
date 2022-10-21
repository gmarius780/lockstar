class BackendSettings:
    """Hardcoded settings for the backend"""
    backend_ip = '0.0.0.0'
    backend_port = 10780
    read_buffer_limit_bytes = 50 * 10 ** 6
    current_module_config_file = './current_module.config'
    mc_communication_speed_Hz = 1000000
    mc_gpio_input_channel = 8
    debug_mode = False
    mc_write_buffer_size = 4096
    mc_internal_clock_rate = 90e6
    mc_max_counter = 65536