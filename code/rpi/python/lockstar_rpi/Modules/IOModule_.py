import logging
from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage

from lockstar_rpi.Modules.Module import Module
from lockstar_general.backend.BackendResponse import BackendResponse


class IOModule_(Module):
    """This Module should not be used directly by the client (hence the _ at the end of the name).
    It contains functionality which is shared by most modules (e.g. linearization of the DAC)
    """
 

    def __init__(self) -> None:
        super().__init__()
        self.calibration = {}


    async def calibrate(self, out_channel: int, out_range_min: float, out_range_max: float, writer):
        """Starts the calibration process of the DAC"""
        self.calibration[out_channel.value] = {
            'out_range_min': out_range_min,
            'out_range_max': out_range_max,
            'calibration': []
        }

        self._mc_set_calibration()

        writer.write(BackendResponse.ACK())
        await writer.drain()

    #==== Linearization Methods START====
    async def set_linearization_one(self, writer, respond=True):
        """Sets the parameters used by the DAC_ONE to linearize the systems response"""
        logging.debug('Backend: set_linearization_one')
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer('uint32_t', 80) # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def linearize_one(self, writer, respond=True):
        """starts the linearization procedure of the microcontroller.

            1. call start_linearization_one on MC
                a) MC performs ramp and records respond
                b) MC sends ACK back
            2. once backend received ACK: call 'get_lin_trace_one'; MC returns the recorded trace
            3. backend calculates the linearization parameters
            4. set_linearization_one
            5. returns recorded trace & linearization to the client
        """
        pass

    #==== Linearization Methods END====

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
        config['calibration'] = self.calibration
        return config

    async def launch_from_config(self, config_dict):
        if 'calibration' in config_dict:
            self.calibration = config_dict['calibration']
            self._mc_set_calibration()