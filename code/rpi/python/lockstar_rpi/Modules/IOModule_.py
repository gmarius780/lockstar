from lockstar_rpi.Modules.Module import Module
from lockstar_general.backend.BackendResponse import BackendResponse


class IOModule_(Module):
    """This Module should not be used directly by the client (hence the _ at the end of the name).
    It contains functionality which is shared by most modules (e.g. calibration of the DAC)
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

    def _mc_set_calibration(self):
        """Writes the calibration to the MC"""
        pass

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
        config['calibration'] = self.calibration
        return config

    async def launch_from_config(self, config_dict):
        if 'calibration' in config_dict:
            self.calibration = config_dict['calibration']
            self._mc_set_calibration()