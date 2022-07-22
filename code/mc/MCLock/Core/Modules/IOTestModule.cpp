/*
 * IOTestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"

#include <main.h>
#include "../HAL/spi.hpp"
#include "../HAL/leds.hpp"
#include "../HAL/adc_new.hpp"
#include "../HAL/dac.hpp"

#ifdef IO_TEST_MODULE

extern ADC_HandleTypeDef hadc3;

class IOTestModule {
public:
	IOTestModule() {

	}
	void run() {
		ADC_Dev = new ADC_Device(	/* SPI number */ 				1,
									/* DMA Stream In */ 			2,
									/* DMA Channel In */ 			3,
									/* DMA Stream Out */ 			3,
									/* DMA Channel Out */ 			3,
									/* conversion pin port */ 		ADC_CNV_GPIO_Port,
									/* conversion pin number */		ADC_CNV_Pin,
									/* Channel 1 config */			ADC_UNIPOLAR_10V,
									/* Channel 2 config */			ADC_UNIPOLAR_10V,
									/* Data buffer size */			1);

		DAC_1 = new DAC_Dev(    /*SPI number*/              6,
								/*DMA Stream Out*/          5,
								/*DMA Channel Out*/         1,
								/*DMA Stream In*/           6,
								/*DMA Channel In*/          1,
								/* sync pin port*/          DAC_1_Sync_GPIO_Port,
								/* sync pin number*/        DAC_1_Sync_Pin,
								/* clear pin port*/         CLR6_GPIO_Port,
								/* clear pin number*/       CLR6_Pin);

		DAC_2 = new DAC_Dev(    /*SPI number*/              5,
								/*DMA Stream Out*/          4,
								/*DMA Channel Out*/         2,
								/*DMA Stream In*/           3,
								/*DMA Channel In*/          2,
								/* sync pin port*/          DAC_2_Sync_GPIO_Port,
								/* sync pin number*/        DAC_2_Sync_Pin,
								/* clear pin port*/         CLR5_GPIO_Port,
								/* clear pin number*/       CLR5_Pin);

		DAC_1->ConfigOutputs(&hadc3, ADC_CHANNEL_14, ADC_CHANNEL_9);
		DAC_2->ConfigOutputs(&hadc3, ADC_CHANNEL_8, ADC_CHANNEL_15);

		float measure = 0;
		float measure2 = 0;
		while(true) {
			ADC_Dev->startConversion();
			measure = ADC_Dev->channel1->getResult();
			measure2 = ADC_Dev->channel2->getResult();
			DAC_1->WriteFloat(measure);
			DAC_2->WriteFloat(measure2);

		}
	}


public:
	ADC_Device *ADC_Dev;
	DAC_Dev *DAC_1, *DAC_2;
};


IOTestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/
void DMA2_Stream4_IRQHandler(void)
{
	module->DAC_2->Callback();
}

void DMA2_Stream5_IRQHandler(void)
{
	module->DAC_1->Callback();
}

void DMA2_Stream2_IRQHandler(void)
{
	// SPI 1 rx
	module->ADC_Dev->DMATransmissionCallback();
}

void DMA2_Stream3_IRQHandler(void)
{
	// SPI 1 tx - SPI 5 rx
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{
	/* To speed up the access to functions, that are often called, we store them in the RAM instead of the FLASH memory.
	 * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
	 * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
	 * */
	extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
	volatile unsigned *src = &__sram_func_loadaddr;
	volatile unsigned *dest = &__sram_func_start;
	while (dest < &__sram_func_end) {
	  *dest = *src;
	  src++;
	  dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new IOTestModule();

	module->run();


}
#endif
