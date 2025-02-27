from lockstar_rpi.Modules.ScopeModule_ import ScopeModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class SinglePIDModule(ScopeModule_):
    """Basic Module which implements a simple PID controller by using input_1 as error_signal, input_2 as setpoint and output 1 for the control signal"""
    def __init__(self) -> None:
        super().__init__()
        self.p = None
        self.i = None
        self.d = None
        self.input_offset = None
        self.output_offset = None
        self.i_threshold = None
        self.intensity_lock_mode = None
        self.out_range_min = None
        self.out_range_max = None
        self.locked = None


    # ==== START: client methods 
    async def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, locked: bool,
                        input_offset: float, output_offset: float, i_threshold: float, intensity_lock_mode: bool, writer):
        """Set all system module parameters

        Args:
        :param    p (float): p
        :param    i (float): i
        :param    d (float): d
        :param    out_range_min (float): output range minimum in volt
        :param    out_range_max (float): output range maximum in volt
        :param    locked (bool): lock
        :param    input_offset (float): voltage to be added to the error-signal (to compensate PD offsets)
        :param    output_offset (float): voltage to be added to the control signal --> e.g. to compensate for 'break-through' voltages in diodes
        :param    writer (_type_): asyncio writer to reply to the client
        """
        logging.debug('Starting initialization: SinglePIDModule')

        #=== sequentially send configuration to MC
        ack = await self.set_pid(p, i, d, input_offset, output_offset, i_threshold, writer, respond=False)
        ack = ack and await self.set_output_limits(out_range_min, out_range_max, writer, respond=False)

        if locked:
            ack = ack and await self.lock(writer, respond=False)
        else:
            ack = ack and await self.unlock(writer, respond=False)
        
        if intensity_lock_mode:
            ack = ack and await self.enable_intensity_lock_mode(writer, respond=False)
        else:
            ack = ack and await self.disable_intensity_lock_mode(writer, respond=False)

        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        if ack:
            self.p = p
            self.i = i
            self.d = d
            self.out_range_min = out_range_min
            self.out_range_max = out_range_max
            self.locked = locked
            self.input_offset = input_offset
            self.output_offset = output_offset
            self.i_threshold = i_threshold
            self.intensity_lock_mode = intensity_lock_mode
        return ack

    async def check_for_ack(self, writer=None):
        ack =  await MC.I().read_ack()
        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
        return ack

    async def set_pid(self, p: float, i: float, d: float, input_offset: float, output_offset: float, i_threshold: float, writer, respond=True):
        """set pid parameters

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
        
        logging.debug('Backend: set_pid')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        mc_data_package.push_to_buffer('float', input_offset)
        mc_data_package.push_to_buffer('float', output_offset)
        mc_data_package.push_to_buffer('float', i_threshold)
        await MC.I().write_mc_data_package(mc_data_package)
        
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.p = p
            self.i = i
            self.d = d
            self.input_offset = input_offset
            self.output_offset = output_offset
            self.i_threshold = i_threshold
        return result

    async def set_output_limits(self, min: float, max: float, writer, respond=True):
        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.out_range_min = min
            self.out_range_max = max
        return result

    async def lock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked = True
        return result

    async def unlock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.locked = False
        return result

    async def enable_intensity_lock_mode(self, writer, respond=True):
        """ Intensity lock mode: i_threshold is considered
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 15) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.intensity_lock_mode = True
        return result

    async def disable_intensity_lock_mode(self, writer, respond=True):
        """ Intensity lock mode: i_threshold is considered
        """
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 16) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        result = await self.check_for_ack(writer=(writer if respond else None))
        if result:
            self.intensity_lock_mode = False
        return result
    
    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
    
        
        return config

    async def launch_from_config(self, config_dict):
        try:
            await super().launch_from_config(config_dict)
            retry_counter = 10
            success = False
            while retry_counter > 0 and not success:
                success = await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
                                config_dict['out_range_max'], config_dict['locked'], config_dict['input_offset'],
                                config_dict['output_offset'], config_dict['i_threshold'], config_dict['intensity_lock_mode'], None)
                retry_counter -= 1
            
        except Exception as ex:
            logging.error(f'SinglePIDModule: canot launch_from_config: {ex}')
            


