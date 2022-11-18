import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall
from lockstar_general.backend.BackendResponse import BackendResponse

import matplotlib.pyplot as plt
import numpy as np

class LinearizationClient(LockstarClient):

	def __init__(self,lockstar_ip,lockstar_port,client_id) -> None:
		super().__init__(lockstar_ip, lockstar_port, client_id)
		self.module_name = 'LinearizationModule'

	def set_ramp_array(self,ramp):
		ramp_list = ramp.tolist()
		bc = BackendCall(self.client_id,self.module_name,'set_ramp_array',args={'ramp':ramp_list})
		return asyncio.run(self._call_lockstar(bc))

	def set_ramp_speed(self,speed):
		bc = BackendCall(self.client_id,self.module_name,'set_ramp_speed',args={'speed':speed})
		return asyncio.run(self._call_lockstar(bc))

	def start_response_measurement(self):
		bc = BackendCall(self.client_id,self.module_name,'start_response_measurement',args={})
		return asyncio.run(self._call_lockstar(bc))

	def measure_response(self):
		bc = BackendCall(self.client_id,self.module_name,'measure_response',args={})
		br = asyncio.run(self._call_lockstar(bc)) ## Maybe only return the reponse itself?

		return np.array(br.response)		


if __name__ == '__main__':
	client = LinearizationClient('192.168.88.220', 10780, 1235)	

	if client.register_client_id():
		logging.info(f'Succesfully initialized Linearization Module')
	else:
		logging.error(f'Linearization module: Could not initialize module')

	ramp = np.linspace(0,5,500)
	client.set_ramp_array(ramp)
	client.set_ramp_speed(1)
