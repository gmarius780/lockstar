##Lockstar installer
# installs lockstar as a linux daemon called lockstar.service
#
# TO RUN IT: sudo python3 ./lockstar_installer.py
#
# @author: Marius Gächter
from os.path import join
import subprocess

service_file_name = '/etc/systemd/system/lockstar.service'
working_directory = '/home/pi/lockstar/code/rpi/python/lockstar_rpi'
daemon_starter_script = 'start_lockstar_daemon.sh'

def write_damon_starter_script():
    with open(join(working_directory, daemon_starter_script), 'w+') as f:
        f.writelines([
            "#!/bin/sh\n",
            "python3 backend.py\n"
        ])

def write_service_file():
    with open(service_file_name, 'w+') as f:
        f.writelines([
            "[Unit]\n",
            "Description=lockstar daemon\n",
            "[Service]\n",
            "User=pi\n",
            f'WorkingDirectory={working_directory}\n',
            f'ExecStart={join(working_directory, daemon_starter_script)}\n',
            "Type=simple\n",
            "TimeoutStopSec=10\n",
            "Restart=on-failure\n",
            "RestartSec=5\n",
            "[Install]\n",
            "WantedBy=multi-user.target\n"
        ])

if __name__ == "__main__":
    print(f'Writing service file: {service_file_name}..')
    write_service_file()
    print(f'Writing daemon starter script: {join(working_directory, daemon_starter_script)}')
    write_damon_starter_script()

    print(f'Installing the lockstar service:')
    print(subprocess.run(['sudo', 'chmod', '+x', join(working_directory, daemon_starter_script)]))
    print(subprocess.run(['sudo','systemctl', 'daemon-reload'], capture_output=True))
    print(subprocess.run(['sudo','systemctl', 'enable', 'lockstar.service'], capture_output=True))
    print(subprocess.run(['sudo','systemctl', 'start', 'lockstar.service'], capture_output=True))
