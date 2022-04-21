from lockstar_general.hardware import HardwareComponents
import logging
def int_to_HardwareComponents(comp):
    try:
        return HardwareComponents(comp)
    except Exception as ex:
        logging.error(f'hardwarecomponent not defined: {ex}')
        raise ex