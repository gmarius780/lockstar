from lockstar_rpi.Modules.IOModule_ import IOModule_
from lockstar_general.backend.BackendResponse import BackendResponse
import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

class SinglePIDModule(IOModule_):
    """Basic Module which implements a simple PID controller by using input_1 as error_signal, input_2 as setpoint and output 1 for the control signal"""
    def __init__(self) -> None:
        super().__init__()
        self.p = None
        self.i = None
        self.d = None
        self.input_offset = None
        self.output_offset = None
        self.out_range_min = None
        self.out_range_max = None
        self.locked = None


    # ==== START: client methods 
    async def initialize(self, p: float, i: float, d: float, out_range_min: float, out_range_max: float, locked: bool,
                        input_offset: float, output_offset: float, writer):
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
        self.p = p
        self.i = i
        self.d = d
        self.out_range_min = out_range_min
        self.out_range_max = out_range_max
        self.locked = locked
        self.input_offset = input_offset
        self.output_offset = output_offset

        logging.debug('Starting initialization: SinglePIDModule')

        #=== sequentially send configuration to MC
        ack = await self.set_pid(p, i, d, input_offset, output_offset, writer, respond=False)
        ack = ack and await self.set_output_limits(out_range_min, out_range_max, writer, respond=False)

        if locked:
            ack = ack and await self.lock(writer, respond=False)
        else:
            ack = ack and await self.unlock(writer, respond=False)

        if writer is not None:
            if ack:
                writer.write(BackendResponse.ACK().to_bytes())
            else:
                writer.write(BackendResponse.NACK().to_bytes())
            await writer.drain()
        
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

    async def set_pid(self, p: float, i: float, d: float, input_offset: float, output_offset: float, writer, respond=True):
        self.p = p
        self.i = i
        self.d = d
        self.input_offset = input_offset
        self.output_offset = output_offset

        logging.debug('Backend: set_pid')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 11) # method_identifier
        mc_data_package.push_to_buffer('float', p) # p
        mc_data_package.push_to_buffer('float', i) # i
        mc_data_package.push_to_buffer('float', d) # d
        mc_data_package.push_to_buffer('float', input_offset)
        mc_data_package.push_to_buffer('float', output_offset)
        await MC.I().write_mc_data_package(mc_data_package)
        
        return await self.check_for_ack(writer=(writer if respond else None))

    async def set_output_limits(self, min: float, max: float, writer, respond=True):
        self.out_range_min = min
        self.out_range_max = max

        logging.debug('Backend: set output limits')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 14) # method_identifier
        mc_data_package.push_to_buffer('float', min)
        mc_data_package.push_to_buffer('float', max)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def lock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 12) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def unlock(self, writer, respond=True):
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 13) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))
    
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
            await super().launch_from_config(config_dict)
            await self.initialize(config_dict['p'], config_dict['i'], config_dict['d'], config_dict['out_range_min'],
                                config_dict['out_range_max'], config_dict['locked'], config_dict['input_offset'],
                                config_dict['output_offset'], None)

            
        except Exception as ex:
            logging.error(f'SinglePIDModule: canot launch_from_config: {ex}')
            raise ex


