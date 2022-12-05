import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_general.backend.BackendResponse import BackendResponse

import matplotlib.pyplot as plt
import numpy as np

class LinearizationClient(LockstarClient):

    def __init__(self,lockstar_ip,lockstar_port,client_id) -> None:
        super().__init__(lockstar_ip, lockstar_port, client_id, 'LinModule')
        self.module_name = 'LinearizationModule'

        self.ramp = None
        self.ramp_start = 0
        self.ramp_end = 0
        self.ramp_length = 0
        self.ramp_speed = 0
        self.measured_gain = 0
        self.inverse_gain = 0

    def set_ramp_parameters(self, ramp_start:float, ramp_end:float, ramp_length:int, settling_time_ms:int):
        bc = BackendCall(self.client_id, self.module_name, 'set_ramp_parameters', 
                        args={
                            'ramp_start': ramp_start, 
                            'ramp_end': ramp_end,
                            'ramp_length': ramp_length,
                            'settling_time_ms': settling_time_ms
                            })
        br = asyncio.run(self._call_lockstar(bc))
        return br.is_ACK()

    def linearize_ch_one(self):
        """starts the linearization procedure of the microcontroller.

            1. MC starts ramp & records the trace
            2. only sends ack when doned
            3. backend calls 'get_measurement_result'
            4. backend calculates linearization
            5. backend calls 'set_linearization_one'
            6. backend returns the measured_gain, and the calculated linearization
        Args:
            ramp_start (_type_): _description_
            ramp_end (_type_): _description_
            nbr_of_ramp_points (_type_): _description_

        Returns:
            _type_: _description_
        """
        bc = BackendCall(self.client_id, self.module_name, 'linearize_ch_one', args={})
        br = asyncio.run(self._call_lockstar(bc))

        print(br.response)
        if br.response != 'NACK' and len(br.response) == 2:
            measured_gain, linearization = br.response
            return measured_gain, linearization
        else:
            return False

    def linearize_ch_two(self):
        """starts the linearization procedure of the microcontroller.

            1. MC starts ramp & records the trace
            2. only sends ack when done
            3. backend calls 'get_measurement_result'
            4. backend calculates linearization
            5. backend calls 'set_linearization_one'
            6. backend returns the measured_gain, and the calculated linearization
        Args:
            ramp_start (_type_): _description_
            ramp_end (_type_): _description_
            nbr_of_ramp_points (_type_): _description_

        Returns:
            _type_: _description_
        """
        bc = BackendCall(self.client_id, self.module_name, 'linearize_ch_two', args={})
        br = asyncio.run(self._call_lockstar(bc))

        measured_gain, linearization = br.response
        return measured_gain, linearization

if __name__ == '__main__':
    client = LinearizationClient('192.168.88.13', 10780, 1234) 

    if client.register_client_id():
        logging.info(f'Succesfully initialized Linearization Module')
    else:
        logging.error(f'Linearization module: Could not initialize module')
    
    ramp_start = 0.
    ramp_end = 10.
    ramp_length = 2000
    settling_time_ms = 1

    ramp = np.linspace(ramp_start, ramp_end, num=ramp_length)

    print(client.set_ramp_parameters(ramp_start, ramp_end, ramp_length, settling_time_ms))
    linearization_response = client.linearize_ch_one()

    if linearization_response != False:
        measured_gain, linearization = linearization_response
        fig,ax = plt.subplots(1,1)
        ax.plot(ramp,measured_gain,label='measured gain')
        ax.plot(ramp,linearization,label='linearizaion')
        ax.legend()
        ax.grid('lightgray')
        plt.show()

        print(client.set_linearization_length_one(ramp_length))
        print(client.set_linearization_one(linearization, min_output_voltage=ramp_start, max_output_voltage=ramp_end))
