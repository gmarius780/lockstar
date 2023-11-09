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
#include "../HAL/DACDevice.hpp"
#include "adc_config.h"
#include "dac_config.h"
#include "../HAL/leds.hpp"
#include <stdio.h>

#ifdef AI_TEST_MODULE
extern ADC_HandleTypeDef hadc3;
volatile uint32_t start_counter = 0;
volatile uint32_t end_counter = 0;
volatile uint32_t diff = 0;
class AITestModule
{
public:
	AITestModule()
	{
	}

	void run()
	{
		ADC_Dev = new ADC_Device(&ADC_conf, ADC_BIPOLAR_10V, ADC_BIPOLAR_10V);
		dac_1 = new DAC1_Device(&DAC1_conf);
		dac_2 = new DAC2_Device(&DAC2_conf);
		dac_1->config_output();
		dac_2->config_output();
		turn_LED2_on();
		turn_LED3_on();
		// ADC_Dev->start_conversion();
		SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
		start_counter = DWT->CYCCNT;
		ADC_Dev->start_conversion();
		m1 = ADC_Dev->channel1->get_result();
		m2 = ADC_Dev->channel2->get_result();
		dac_1->write(m1);
		dac_2->write(m1);
		end_counter = DWT->CYCCNT;

		diff = end_counter - start_counter;
		__asm__ __volatile__("" ::"m"(diff));
		while (true)
		{
			// start = CYCCNT;
			// ADC_Dev->start_conversion();
			// m1 = ADC_Dev->channel1->get_result();
			// m2 = ADC_Dev->channel2->get_result();
			// dac_1->write(m1);
			// dac_2->write(m1);
			// end = CYCCNT;
			// printf("Channel 1: %f\n", m1);
			// printf("Channel 2: %f\n", m2);
		}
	}

public:
	ADC_Device *ADC_Dev;
	DAC1_Device *dac_1;
	DAC2_Device *dac_2;
	float m1 = 0;
	float m2 = 0;
};

AITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/
/********************
||      DAC1      ||
********************/
void BDMA_Channel1_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}

void SPI6_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}
/********************
||      DAC2      ||
********************/
void DMA2_Stream3_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}
void SPI5_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}
/********************
||       ADC       ||
********************/
void DMA1_Stream4_IRQHandler(void)
{
	module->ADC_Dev->dma_receive_callback();
}

void DMA1_Stream5_IRQHandler(void)
{
	module->ADC_Dev->dma_transmission_callback();
}

// void SPI2_IRQHandler(void)
// {
// 	module->ADC_Dev->spi_transmision_callback();
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

	module = new AITestModule();

	module->run();
}
#endif
