from os.path import join, dirname
import logging
import numpy as np
import matplotlib.pyplot as plt
from lockstar_client.SinglePIDClient import SinglePIDClient
from lockstar_client.LinearizationClient import LinearizationClient


linearization_file = join(dirname(__file__), 'test_linearization.json')
linearization_length = 2000

lin_ramp_start = 0.
lin_ramp_end = 10.
lin_settling_time_ms = 1

lin_ramp = np.linspace(lin_ramp_start, lin_ramp_end, num=linearization_length)


lockstar_ip = '192.168.88.201'
client_id = 12345

linearization_client = LinearizationClient(lockstar_ip, 10780, client_id)
pid_client = SinglePIDClient(lockstar_ip, 10780, client_id)

def perform_linearization():
    print(linearization_client.set_ramp_parameters(lin_ramp_start, lin_ramp_end, linearization_length, lin_settling_time_ms))
    linearization_response = linearization_client.linearize_ch_one()

    if linearization_response != False:
        measured_gain, linearization = linearization_response
        fig,ax = plt.subplots(1,1)
        ax.plot(lin_ramp,measured_gain,label='measured gain')
        ax.plot(lin_ramp,linearization,label='linearizaion')
        ax.legend()
        ax.grid('lightgray')
        plt.show()

        linearization_client.store_linearization_locally(linearization, lin_ramp_start, lin_ramp_end, linearization_file)

def configure_linearization():
    pid_client.set_linearization_length_one(linearization_length)
    pid_client.set_linearization_one_from_file(linearization_file)

def initialize_pid():
    pid_client.set_ch_one_output_limits(0., 10.)
    pid_client.set_pid(0, 10000, 0, 0, 0)

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler("./debug.log"),
            logging.StreamHandler()
        ]
    )
    #initialize clients
    pid_client.register_client_id()

    initialize_pid()
    
    