/*
 * AITestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32h725xx.h"
#include "../HAL/ADCDevice.hpp"
#include "../HAL/leds.hpp"

#ifdef AI_TEST_MODULE


class AITestModule {
public:
	AITestModule() {

	}

	void run() {
		ADC_Dev = new ADC_Device(	/* SPI number */ 				3,
									/* DMA Stream In */ 			2,
									/* DMA Channel In */ 			3,
									/* DMA Stream Out */ 			3,
									/* DMA Channel Out */ 			3,
									/* conversion pin port */ 		ADC_CNV_GPIO_Port,
									/* conversion pin number */		ADC_CNV_Pin,
									/* Channel 1 config */			ADC_UNIPOLAR_10V,
									/* Channel 2 config */			ADC_UNIPOLAR_10V);


		float m1 = 0;

		while(true) {
			ADC_Dev->start_conversion();

			m1 = ADC_Dev->channel1->get_result();
            turn_LED2_on();
            turn_LED3_on();

		}
	}


public:
	ADC_Device *ADC_Dev;
};


AITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/


//__attribute__((section("sram_func")))
void DMA1_Stream3_IRQHandler(void)
{
	// SPI 1 rx
	module->ADC_Dev->dma_transmission_callback();
}

void DMA1_Stream4_IRQHandler(void)
{
    // SPI 1 tx
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

	module = new AITestModule();

	module->run();


}
#endif
