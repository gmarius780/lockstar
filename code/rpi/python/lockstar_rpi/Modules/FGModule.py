from lockstar_rpi.MC import MC
from lockstar_rpi.MCDataPackage import MCDataPackage
from lockstar_rpi.Modules.BufferBaseModule_ import BufferBaseModule_
import logging
import ctypes


class FGModule(BufferBaseModule_):
    """Allows user to upload arbitrary waveforms into two buffers (corresponding to the two analog outputs), splitting
    these waveforms into 'chunks' and outputting those chunks either via digital trigger or by calling output_on()
    """

    def __init__(self) -> None:
        super().__init__()

    # ==== START: client methods
    async def set_cfunction(self, func: str, writer, respond=True):
        match func:
            case "cos":
                ll_func = ctypes.c_uint32(0)
            case "sin":
                ll_func = ctypes.c_uint32(1)
            case "phase":
                ll_func = ctypes.c_uint32(2)
            case "mod":
                ll_func = ctypes.c_uint32(3)
            case "arctan":
                ll_func = ctypes.c_uint32(4)
            case "cosh":
                ll_func = ctypes.c_uint32(5)
            case "sinh":
                ll_func = ctypes.c_uint32(6)
            case "arctanh":
                ll_func = ctypes.c_uint32(7)
            case "ln":
                ll_func = ctypes.c_uint32(8)
            case "sqrt":
                ll_func = ctypes.c_uint32(9)
            case _:
                ll_func = ctypes.c_uint32(0)

        """Set Cordic function"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer(
            "uint32_t", ctypes.c_uint32(31)
        )  # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def start_ccalculation(self, writer, respond=True):
        """Start Cordic computation"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer(
            "uint32_t", ctypes.c_uint32(32)
        )  # method_identifier
        await MC.I().write_mc_data_package(mc_data_package)
        return await self.check_for_ack(writer=(writer if respond else None))

    async def start_output(self, writer, respond=True):
        """start output"""
        mc_data_package = MCDataPackage()
        mc_data_package.push_to_buffer(
            "uint32_t", ctypes.c_uint32(32)
        )  # method_identifier
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
