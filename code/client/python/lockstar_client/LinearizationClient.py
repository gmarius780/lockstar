import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_general.backend.BackendResponse import BackendResponse
import json

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

    async def store_linearization_locally(self, linearization, min_output_voltage, max_output_voltage, local_file):
        """ Stores the linearization in a local json file such that it can be set later using the function LockstarClient.set_linearization_from_file
        """
        with open(local_file, 'w+') as f:
            json.dump({
                'linearization': linearization, 'min_output_voltage': min_output_voltage, 'max_output_voltage': max_output_voltage
            }, f)

    async def set_ramp_parameters(self, ramp_start:float, ramp_end:float, ramp_length:int, settling_time_ms:int):
        bc = BackendCall(self.client_id, self.module_name, 'set_ramp_parameters', 
                        args={
                            'ramp_start': ramp_start, 
                            'ramp_end': ramp_end,
                            'ramp_length': ramp_length,
                            'settling_time_ms': settling_time_ms
                            })
        br = await self._call_lockstar(bc)
        return br.is_ACK()

    async def linearize_ch_one(self):
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
        br = await self._call_lockstar(bc)

        if br.response != 'NACK' and len(br.response) == 2:
            measured_gain, linearization = br.response
            return measured_gain, linearization
        else:
            return False

    async def linearize_ch_two(self):
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
        br = await self._call_lockstar(bc)

        measured_gain, linearization = br.response
        return measured_gain, linearization

async def main():
    from os.path import join, dirname
    client = LinearizationClient('192.168.137.2', 10780, 1234)

    await client.set_ch_one_output_limits(0, 10)
    await client.set_ch_two_output_limits(0, 10)
    await client.unclamp_output()

    if await client.register_client_id():
        logging.info(f'Succesfully initialized Linearization Module')
    else:
        logging.error(f'Linearization module: Could not initialize module')
    
    ramp_start = 0.
    ramp_end = 10.
    ramp_length = 2000
    settling_time_ms = 1

    linearization_file = join(dirname(__file__), 'test_linearization.json')
    # print(await client.set_linearization_length_one(ramp_length))
    # print(await client.set_linearization_one_from_file(linearization_file))
    # sys.exit()
    ramp = np.linspace(ramp_start, ramp_end, num=ramp_length)

    print(await client.set_ramp_parameters(ramp_start, ramp_end, ramp_length, settling_time_ms))
    linearization_response = await client.linearize_ch_one()

    if linearization_response != False:
        measured_gain, linearization = linearization_response
        fig,ax = plt.subplots(1,1)
        ax.plot(ramp,measured_gain,label='measured gain')
        ax.plot(ramp,linearization,label='linearizaion')
        ax.legend()
        ax.grid('lightgray')
        plt.show()

        await client.store_linearization_locally(linearization, ramp_start, ramp_end, linearization_file)

        print(await client.set_linearization_length_one(ramp_length))
        #print(await client.set_linearization_one(linearization, min_output_voltage=ramp_start, max_output_voltage=ramp_end))
        print(await client.set_linearization_one_from_file(linearization_file))


if __name__ == '__main__':
    asyncio.run(main())