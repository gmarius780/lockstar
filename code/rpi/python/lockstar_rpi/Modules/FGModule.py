from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
from lockstar_rpi.Modules.BufferBaseModule_ import BufferBaseModule_
import logging
import ctypes
from enum import Enum

from numpy import uint32
from numpy import int32

class CordicScale(Enum):
    LL_CORDIC_SCALE_0 = 0x00000000
    LL_CORDIC_SCALE_1 = 0x00000100
    LL_CORDIC_SCALE_2 = 0x00000200
    LL_CORDIC_SCALE_3 = 0x00000300
    LL_CORDIC_SCALE_4 = 0x00000400
    LL_CORDIC_SCALE_5 = 0x00000500
    LL_CORDIC_SCALE_6 = 0x00000600
    LL_CORDIC_SCALE_7 = 0x00000700

class FGModule(BufferBaseModule_):
    """Allows user to upload arbitrary waveforms into two buffers (corresponding to the two analog outputs), splitting
    these waveforms into 'chunks' and outputting those chunks either via digital trigger or by calling output_on()
    """

    def __init__(self) -> None:
        super().__init__()

    # ==== START: client methods
    async def set_cfunction(self, func: str, scale: str, writer, respond=True):
        match func:
            case "cos":
                ll_func = 0
            case "sin":
                ll_func = 1
            case "phase":
                ll_func = 2
            case "mod":
                ll_func = 3
            case "arctan":
                ll_func = 4
            case "cosh":
                ll_func = 5
            case "sinh":
                ll_func = 6
            case "arctanh":
                ll_func = 7
            case "ln":
                ll_func = 8
            case "sqrt":
                ll_func = 9
            case _:
                ll_func = 0

        for cordic_scale in CordicScale:
            if cordic_scale.name == scale:
                ll_scale = cordic_scale.value

        """Set Cordic function"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer("uint32_t", 31)  # method_identifier
        mc_data_package.push_to_buffer("uint32_t", ll_func)
        mc_data_package.push_to_buffer("uint32_t", ll_scale)
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def start_ccalculation(
        self,
        total_scaling : float,
        offset,
        num_samples,
        start_value,
        step_size,
        writer,
        respond=True,
    ):
        """Start Cordic computation"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer("uint32_t", 32)  # method_identifier
        mc_data_package.push_to_buffer("float", total_scaling)
        mc_data_package.push_to_buffer("uint32_t", offset)
        mc_data_package.push_to_buffer("uint32_t", num_samples)
        mc_data_package.push_to_buffer("int32_t", int32(start_value))
        mc_data_package.push_to_buffer("int32_t", int32(step_size))

        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def start_output(self, writer, respond=True):
        """start output"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer("uint32_t", 33)  # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    # ==== END: client methods

    def generate_config_dict(self):
        """Stores all the relevant information in a dictionary such that the module can be relaunched with this information"""
        config = super().generate_config_dict()
        return config

    async def launch_from_config(self, config_dict):
        try:
            await super().launch_from_config(config_dict)
        except Exception as ex:
            logging.error(f"FGModule: canot launch_from_config: {ex}")
