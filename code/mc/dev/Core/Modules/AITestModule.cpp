/*
 * AITestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../HAL/ADCDevice.hpp"
#include "../HAL/leds.hpp"
#include <stdio.h>

#ifdef AI_TEST_MODULE

class AITestModule
{
public:
	AITestModule()
	{
	}

	void run()
	{
		ADC_Dev = new ADC_Device(/* SPI number */ 3,
								 /* DMA Stream In */ 2,
								 /* DMA Channel In */ 3,
								 /* DMA Stream Out */ 3,
								 /* DMA Channel Out */ 3,
								 //  /* conversion pin port */ SPI2_NSS_GPIO_Port,
								 //  /* conversion pin number */ SPI2_NSS_Pin,
								 ADC_CNV_GPIO_Port,
								 ADC_CNV_Pin,
								 /* Channel 1 config */ ADC_BIPOLAR_5V,
								 /* Channel 2 config */ ADC_BIPOLAR_5V);

		turn_LED2_on();
		turn_LED3_on();

		// ADC_Dev->start_conversion();
		while (true)
		{
			ADC_Dev->start_conversion();
			// m1 = ADC_Dev->channel1->get_result();
			// m2 = ADC_Dev->channel2->get_result();
			// printf("Channel 1: %f\n", m1);
			// printf("Channel 2: %f\n", m2);
		}
	}

public:
	ADC_Device *ADC_Dev;
	float m1 = 0;
	float m2 = 0;
};

AITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/

void DMA1_Stream2_IRQHandler(void)
{
	if (LL_DMA_IsActiveFlag_TC2(DMA1))
	{
		LL_DMA_ClearFlag_TC2(DMA1);
		module->ADC_Dev->dma_receive_callback();
	}
}

void DMA1_Stream3_IRQHandler(void)
{
	if (LL_DMA_IsActiveFlag_TC3(DMA1))
	{
		LL_DMA_ClearFlag_TC3(DMA1);
		module->ADC_Dev->dma_transmission_callback();
	}
}

void DMA1_Stream4_IRQHandler(void)
{
	if (LL_DMA_IsActiveFlag_TC4(DMA1))
	{
		LL_DMA_ClearFlag_TC4(DMA1);
		module->ADC_Dev->dma_receive_callback();
	}
}

void DMA1_Stream5_IRQHandler(void)
{
	if (LL_DMA_IsActiveFlag_TC5(DMA1))
	{
		LL_DMA_ClearFlag_TC5(DMA1);
		module->ADC_Dev->dma_transmission_callback();
	}
}

void SPI2_IRQHandler(void)
{
	/* Check EOT flag value in ISR register */
	if (LL_SPI_IsActiveFlag_EOT(SPI2))
	{
		LL_SPI_ClearFlag_EOT(SPI2);
		module->ADC_Dev->spi_transmision_callback();
	}
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
	while (dest < &__sram_func_end)
	{
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
