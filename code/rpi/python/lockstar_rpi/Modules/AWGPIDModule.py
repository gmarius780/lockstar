from lockstar_rpi.Modules.BufferBaseModule_ import BufferBaseModule_
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class AWGPIDModule(BufferBaseModule_):
    """ Allows user to upload arbitrary waveforms into two buffers (corresponding to the two analog outputs), splitting
    these waveforms into 'chunks' and using those waveforms to ramp setpoints of two pid-controllers. (either via digital trigger or by calling output_on())
    """

    def __init__(self) -> None:
        super().__init__()
        self.p_one = None
        self.i_one = None
        self.d_one = None
        self.p_two = None
        self.i_two = None
        self.d_two = None
        self.input_offset_one = None
        self.output_offset_one = None
        self.i_threshold_one = None
        self.input_offset_two = None
        self.output_offset_two = None
        self.i_threshold_two = None
        self.intensity_lock_mode_one = False
        self.intensity_lock_mode_two = False

        self.locked = None


    # ==== START: client methods 

    async def set_pid_one(self, p: float, i: float, d: float, input_offset: float, output_offset: float, i_threshold: float, writer, respond=True):
        """Set pid parameters corresponding to output one

        Args:
            p (float): 
            i (float): 
            d (float): 
            input_offset (float): added to the error signal before calculating the output
            output_offset (float): added to the output of the controller
            i_threshold (float): if the photodiode signal is bellow this, the integrator remains zero
            writer (_type_): _description_
            respond (bool, optional): Only false if called by launch_from_config. Defaults to True.

        Returns:
            _type_: _description_
        """

        logging.debug('Backend: set_pid_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 31) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        mc_data_package.push_to_buffer('float', input_offset)
        mc_data_package.push_to_buffer('float', output_offset)
        mc_data_package.push_to_buffer('float', i_threshold)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.p_one = p
            self.i_one = i
            self.d_one = d
            self.input_offset_one = input_offset
            self.output_offset_one = output_offset
            self.i_threshold_one = i_threshold
        return result
        

    async def set_pid_two(self, p: float, i: float, d: float, input_offset: float, output_offset: float, i_threshold: float, writer, respond=True):
        """Set pid parameters corresponding to output two

        Args:
            p (float): 
            i (float): 
            d (float): 
            input_offset (float): added to the error signal before calculating the output
            output_offset (float): added to the output of the controller
            i_threshold (float): if the photodiode signal is bellow this, the integrator remains zero
            writer (_type_): _description_
            respond (bool, optional): Only false if called by launch_from_config. Defaults to True.

        Returns:
            _type_: _description_
        """

        logging.debug('Backend: set_pid_two')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 32) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        mc_data_package.push_to_buffer('float', input_offset)
        mc_data_package.push_to_buffer('float', output_offset)
        mc_data_package.push_to_buffer('float', i_threshold)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.p_two = p
            self.i_two = i
            self.d_two = d
            self.input_offset_two = input_offset
            self.output_offset_two = output_offset
            self.i_threshold_two = i_threshold
        return result

    async def lock(self, writer, respond=True):
        """Lock both PID loops
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 33) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def unlock(self, writer, respond=True):
        """Unlock both PID loops"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 34) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))
    
    async def enable_intensity_lock_mode_one(self, writer, respond=True):
        """ Intensity lock mode: i_threshold is considered
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 35) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.intensity_lock_mode_one = True
        return result

    async def disable_intensity_lock_mode_one(self, writer, respond=True):
        """ Intensity lock mode: i_threshold is considered
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 36) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.intensity_lock_mode_one = False
        return result
    
    async def enable_intensity_lock_mode_two(self, writer, respond=True):
        """ Intensity lock mode: i_threshold is considered
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 37) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.intensity_lock_mode_two = True
        return result

    async def disable_intensity_lock_mode_two(self, writer, respond=True):
        """ Intensity lock mode: i_threshold is considered
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 38) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.intensity_lock_mode_two = False
        return result
    
    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()

        return config

    async def launch_from_config(self, config_dict):
        try:
            await  super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f'AWGModule: canot launch_from_config: {ex}')


