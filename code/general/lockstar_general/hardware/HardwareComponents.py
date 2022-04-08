from enum import Enum

#TODO: implement in C++

class HardwareComponents(Enum):
    """Contains human readable mappings to hardware components on the lockstar board"""

    analog_in_one = 0
    analog_in_two = 1
    analog_out_one = 2
    analog_out_two = 3

    @staticmethod
    def from_int(int_component):
        for component in HardwareComponents:
            if component.value == int_component:
                return component
        
        raise ValueError(f'HardwareComponents component unknown: {int_component}')
