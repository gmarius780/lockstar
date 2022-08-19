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
