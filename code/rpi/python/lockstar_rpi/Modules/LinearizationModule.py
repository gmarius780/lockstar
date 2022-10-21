import numpy as np

class LinearizationModule():

	NUMBER_OF_PIVOT_POINTS = 101
	RAMP_START = 0
	RAMP_END = 10


	def __init__(self) -> None:
		super().__init__()
		self.measured_response = np.zeros((NUMBER_OF_PIVOT_POINTS))

	# ==== START: client methods

	async def initialize(self, writer):
		pass

	"""
	Signals the uC to output the specified ramp and return the
	measured system response
	"""
	async def start_linearization(self, writer, respond=True):
		logging.debug('Backend: start_linearization')
		mc_data_package = MCDataPackage()
		mc_data_package.push_to_buffer('uint32_t', 11) ## S: why all start with 11?
		mc_data_package.push_to_buffer('float', RAMP_START)
		mc_data_package.push_to_buffer('float', RAMP_END)
		mc_data_package.push_to_buffer('uint32_t', NUMBER_OF_PIVOT_POINTS)

		await MC.I().write_mc_data_package(mc_data_package)
		ack = await self.check_for_ack(writer=(writer if respond else None))
		if not ack:
			logging.error(f'start linearization: could not initializer linearization!')
			await writer.drain()
			return False

		# TODO: add timeout
		length,measured_points = await MC.I().read_mc_data()
		if length != NUMBER_OF_PIVOT_POINTS:
			logging.error(f'start linearization: Number of measured points incorrect!')
			await writer.drain()
			return False

		file_writer = open('response.txt', 'a')
		for i,p in enumerate(measured_points):
			measured_response[i] = p
			file_writer.write(str(p)+'\n')
			
		return True
			
		
		

		


