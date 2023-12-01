#ifdef AI_TEST_MODULE
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
#include "../HAL/BasicTimer.hpp"
#include "adc_config.h"
#include "dac_config.h"
#include "../HAL/leds.hpp"

uint32_t start_cycle, end_cycle, total_cycles;
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

		prescaler = 275;
		counter_max = 1000;
		this->sampling_timer = new BasicTimer(2, counter_max, prescaler);

		dac_1->write(0);
		dac_2->write(0);
		sampling_timer->enable_interrupt();
		sampling_timer->enable();

		while (true)
		{
		}
	}
	void sampling_timer_interrupt()
	{
		ADC_Dev->start_conversion();
		m1 = ADC_Dev->channel1->get_result();
		m2 = ADC_Dev->channel2->get_result();
		dac_1->write(m1);
		dac_2->write(m2);
	}

public:
	ADC_Device *ADC_Dev;
	DAC1_Device *dac_1;
	DAC2_Device *dac_2;
	float m1 = 0;
	float m2 = 0;
	BasicTimer *sampling_timer;
	uint32_t counter_max, prescaler;
};

AITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/

/********************
||      DAC1      ||
********************/
__attribute__((section(".itcmram"))) void BDMA_Channel1_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}
__attribute__((section(".itcmram"))) void SPI6_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}
/********************
||      DAC2      ||
********************/
__attribute__((section(".itcmram"))) void DMA2_Stream3_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}
__attribute__((section(".itcmram"))) void SPI5_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}

/********************
||      TIM2       ||
********************/
__attribute__((section(".itcmram"))) void TIM2_IRQHandler(void)
{
	LL_TIM_ClearFlag_UPDATE(TIM2);
	module->sampling_timer_interrupt();
}
/********************
||       ADC       ||
********************/
__attribute__((section(".itcmram"))) void DMA1_Stream4_IRQHandler(void)
{
	module->ADC_Dev->dma_receive_callback();
}

__attribute__((section(".itcmram"))) void DMA1_Stream5_IRQHandler(void)
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
	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new AITestModule();

	module->run();
}
#endif
