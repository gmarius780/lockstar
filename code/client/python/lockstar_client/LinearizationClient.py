import asyncio
import logging
from lockstar_client.LockstarClient import LockstarClient
from lockstar_general.backend.BackendCall import BackendCall

class LinearizationClient(LockstarClient):

	def __init__(self,lockstar_ip,lockstar_port,client_id) -> None:
		super().__init__(lockstar_ip, lockstar_port, client_id)
		self.module_name = 'LinearizationModule'

	def start_linearization(self):
		bc = BackendCall(self.client_id,self.module_name,'start_linearization',args={})
		return asyncio.run(self._call_lockstar(bc))

	def get_measurement(self):
		bc = BackendCall(self.client_id,self.module_name,'get_measurement',args={})
		return asyncio.run(self._call_lockstar(bc))

if __name__ == '__main__':
	client = LinearizationClient('192.168.88.220', 10780, 1235)	

	if client.register_client_id():
		logging.info(f'Succesfully initialized Linearization Module')
	else:
		logging.error(f'Linearization module: Could not initialize module')