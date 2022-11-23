/*
 * Module.cpp
 *
 *  Created on: Feb 25, 2022
 *      Author: qo
 */

#include "Module.hpp"

extern ADC_HandleTypeDef hadc3;

Module::Module() {

}

Module::~Module() {
	// TODO Auto-generated destructor stub
}

/*
 * Checks if the called method is implemented in this class. If not it returns false such that
 * the derived classes know that they need to check, if they know the method
 */
bool Module::handle_rpi_base_methods() {
	/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
	RPIDataPackage* read_package = rpi->get_read_package();
	switch (read_package->pop_from_buffer<uint32_t>()) {
	case METHOD_SET_LINEARIZATION_ONE:
		set_linearization_one(read_package);
		break;
	default:
		return false;
	}

	return true;
}

void Module::initialize_adc(uint8_t ch1_config, uint8_t ch2_config) {
	adc = new ADC_Device(	/* SPI number */ 				1,
							/* DMA Stream In */ 			2,
							/* DMA Channel In */ 			3,
							/* DMA Stream Out */ 			3,
							/* DMA Channel Out */ 			3,
							/* conversion pin port */ 		ADC_CNV_GPIO_Port,
							/* conversion pin number */		ADC_CNV_Pin,
							/* Channel 1 config */			ch1_config,
							/* Channel 2 config */			ch2_config);
}

void Module::initialize_dac() {
	dac_1 = new DAC_Device( /*SPI number*/              6,
							/*DMA Stream Out*/          5,
							/*DMA Channel Out*/         1,
							/* sync pin port*/          DAC_1_Sync_GPIO_Port,
							/* sync pin number*/        DAC_1_Sync_Pin,
							/* clear pin port*/         CLR6_GPIO_Port,
							/* clear pin number*/       CLR6_Pin);

	dac_2 = new DAC_Device( /* SPI number*/				5,
							/* DMA Stream Out*/			4,
							/* DMA Channel Out*/		2,
							/* sync pin port*/          DAC_2_Sync_GPIO_Port,
							/* sync pin number*/        DAC_2_Sync_Pin,
							/* clear pin port*/         CLR5_GPIO_Port,
							/* clear pin number*/       CLR5_Pin);

	dac_1->config_output(&hadc3, ADC_CHANNEL_14, ADC_CHANNEL_9);
	dac_2->config_output(&hadc3, ADC_CHANNEL_8, ADC_CHANNEL_15);
}

void Module::initialize_rpi() {
	rpi = new RPI();
}

void Module::set_ch_output_limit(RPIDataPackage* read_package, DAC_Device *dac) {
	/***Read arguments***/
	dac->set_min_output(read_package->pop_from_buffer<float>());
	dac->set_max_output(read_package->pop_from_buffer<float>());

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}

/*LINEARIZATION-METHODS START*/
void Module::set_linearization_one(RPIDataPackage* read_package) {

}
/*LINEARIZATION-METHODS END*/

