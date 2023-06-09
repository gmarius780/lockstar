#==================================================================================================================
#=== This script does the following: ===
# 1. Create the .elf microcontroller images for all the configured modules
# 2. Copy the .elf files into /rpi/python/Modules/mc_images
# 3. Create a .cfg file for each module in /rpi/python/Modules/mc_images, which will allow the rpi to flash the mc
#
#=== How to use this script: ===
# Add a module:
#
# Run the compilation:
#
#
# @author: Marius Gächter
#==================================================================================================================

from os.path import join, dirname

modules = [
    {
        'module_name': 'SinglePIDModule',
        'compiler_flag': 'SINGLE_PID_MODULE'
    },
    {
        'module_name': 'AWGModule',
        'compiler_flag': 'AWG_MODULE'
    },
     {
        'module_name': 'LinearizationModule',
        'compiler_flag': 'LINEARIZATION_MODULE'
    },
         {
        'module_name': 'AnalogOutputModule',
        'compiler_flag': 'LINEARIZATION_MODULE'
    }
]

output_folder = join(dirname(__file__), '..', 'rpi', 'python', 'lockstar_rpi', 'Modules', 'mc_images')
raspberry_elf_directory = '/home/pi/lockstar/code/rpi/python/lockstar_rpi/Modules/mc_images'

def write_cfg_file_for_module(config_dict, output_folder):
    outfile_name = f'{config_dict["module_name"]}.cfg'

    elf_file = join(raspberry_elf_directory, f'{config_dict["module_name"]}.elf')

    with open(join(output_folder, outfile_name), 'w+') as f:
        f.writelines([
            "source [find interface/sysfsgpio-raspberrypi.cfg]\n",
            "transport select swd\n",
            "set WORKAREASIZE 0x2000\n",
            "source [find target/stm32f4x.cfg]\n",
            "adapter_nsrst_delay 100\n",
            "adapter_nsrst_assert_width 100\n",
            "init\n",
            "targets\n",
            "reset halt\n",
            f'program {elf_file} verify\n',
            "reset\n",
            "shutdown\n"
        ])

def create_elf_file_for_module(config_dict, output_folder):
    """not yet implemetned. Crate the .elf file from the stm32 ide

    Args:
        config_dict (_type_): _description_
        output_folder (_type_): _description_
    """
    # use subprocess
    pass

if __name__ == "__main__":
    print(f'Starting compilation of {len(modules)} modules...')

    for config_dict in modules:
        print(f'starting compilation of {config_dict["module_name"]}')
        create_elf_file_for_module(config_dict, output_folder)
        write_cfg_file_for_module(config_dict, output_folder)
    
    print('Done!')
