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

		self.ramp = None
		self.ramp_speed = 0
		self.measured_gain = 0
		self.inverse_gain = 0

	def new_linearization(self):
		ramp_list = self.ramp.tolist()
		bc = BackendCall(self.client_id,self.module_name,'new_linearization',args={'ramp':ramp_list,'ramp_speed':self.ramp_speed})

		br = asyncio.run(self._call_lockstar(bc)) ## Maybe only return the reponse itself?
		self.measured_gain = br.response[0]
		self.inverse_gain = br.response[1]
		
		fig,ax = plt.subplots(1,1)
		ax.plot(self.ramp,self.measured_gain,label='measured gain')
		ax.plot(self.ramp,self.inverse_gain,label='inverse gain')
		ax.legend()
		ax.grid('lightgray')
		plt.savefig('meas.png')
		plt.show()


if __name__ == '__main__':
	client = LinearizationClient('192.168.88.201', 10780, 1235)	

	if client.register_client_id():
		logging.info(f'Succesfully initialized Linearization Module')
	else:
		logging.error(f'Linearization module: Could not initialize module')

	client.ramp = np.linspace(0,5,200)
	client.ramp_speed = 1
	client.new_linearization()
