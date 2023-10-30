/*
 * AOTestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../HAL/DACDevice.hpp"
#include "../HAL/leds.hpp"
#include <stdio.h>
#include "../Modules/dac_config.h"

#ifdef AO_TEST_MODULE

extern ADC_HandleTypeDef hadc3;

class AOTestModule
{
public:
	AOTestModule()
	{
	}

	void run()
	{
		// DAC_2 = new DAC_Device( /*SPI number*/              6,
		// 						/*DMA Stream Out*/          5,
		// 						/*DMA Channel Out*/         1,
		// 						/* sync pin port*/          DAC_2_Sync_GPIO_Port,
		// 						/* sync pin number*/        DAC_2_Sync_Pin,
		// 						/* clear pin port*/         DAC2_CLEAR_PORT,
		// 						/* clear pin number*/       DAC2_CLEAR_PIN);
		// DAC_2 = new DAC_Device( /*SPI number*/              6,
		// 						/*DMA Stream Out*/          5,
		// 						/*DMA Channel Out*/         1,
		// 						/* sync pin port*/          DAC2_SYNC_PORT,
		// 						/* sync pin number*/        DAC2_SYNC_PIN,
		// 						/* clear pin port*/         DAC2_CLEAR_PORT,
		// 						/* clear pin number*/       DAC2_CLEAR_PIN);
		DAC_1 = new DAC_Device( 
						/* sync pin port*/          DAC2_SYNC_PORT,
						/* sync pin number*/        DAC2_SYNC_PIN,
						/* clear pin port*/         DAC1_CLEAR_PORT,
						/* clear pin number*/       DAC1_CLEAR_PIN);                                

		turn_LED2_on();
		turn_LED3_on();

		float m1 = 6;
        // DAC_2->config_output(&hadc3, DAC2_SENL, DAC2_SENH);
		// DAC_2->write(m1);
		DAC_1->config_output(&hadc3, DAC2_SENL, DAC2_SENH);
		DAC_1->write(m1);
		while (true)
		{
			// ADC_Dev->start_conversion();
			// m1 = ADC_Dev->channel1->get_result();
			// m2 = ADC_Dev->channel2->get_result();
			// printf("Channel 1: %f\n", m1);
			// printf("Channel 2: %f\n", m2);
		}
	}

public:
	DAC_Device *DAC_1;
	DAC_Device *DAC_2;
	float m1 = 0;
	float m2 = 0;
};

AOTestModule *module;

// /******************************
//  *         INTERRUPTS          *
//  *******************************/
// void DMA1_Stream2_IRQHandler(void)
// {
// 	module->ADC_Dev->dma_receive_callback();
// }

void DMA1_Stream3_IRQHandler(void)
{
	module->DAC_2->dma_transmission_callback();
}

void DMA2_Stream3_IRQHandler(void)
{
	module->DAC_2->dma_transmission_callback();
}

void BDMA_Channel1_IRQHandler(void)
{
	module->DAC_1->dma_transmission_callback();
}


// void DMA1_Stream4_IRQHandler(void)
// {
// 	module->ADC_Dev->dma_receive_callback();
// }

// void DMA1_Stream5_IRQHandler(void)
// {
// 	module->ADC_Dev->dma_transmission_callback();
// }

// void SPI2_IRQHandler(void)
// {
// 	LL_SPI_ClearFlag_EOT(SPI2);
// 	// LL_SPI_SetReloadSize(SPI2, DATAWIDTH);
// 	LL_SPI_StartMasterTransfer(SPI2);
// }

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
	while (dest < &__sram_func_end)
	{
		*dest = *src;
		src++;
		dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new AOTestModule();

	module->run();
}
#endif
