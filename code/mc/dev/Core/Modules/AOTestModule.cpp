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
#include "sin_wave.h"

#ifdef AO_TEST_MODULE

#define CYCTEST
#define PROBE_SPI
#include "dac_config.h"
extern ADC_HandleTypeDef hadc3;

class AOTestModule
{
public:
	AOTestModule()
	{
	}

	void run()
	{
		DAC_1 = new DAC1_Device(&DAC1_conf);
		DAC_2 = new DAC2_Device(&DAC2_conf);
		uint32_t i = 0;
		bool isCountingUp = true;
		float m1 = 0;
		float m2 = 8;
		DAC_1->config_output();
		// DAC_1->write(m1);
		// turn_LED2_on();
		DAC_2->config_output();
		// DAC_2->write(m2);
		// turn_LED3_on();
#ifdef PROBE_SPI
		DAC_3 = new DAC2_Device(&DAC3_conf);
		// DAC_3->config_output();
		// DAC_3->write(m2);
#endif
		while (true)
		{
			// if(i < 1023)
			// 	DAC_2->write(sin_wave[i++]);
			// else{
			// 	i = 0;
			// 	DAC_2->write(sin_wave[i++]);
			// }
#ifdef CYCTEST
			if (isCountingUp)
			{
				if (m1 < 9)
				{
					m1 += 0.1;
				}
				else
				{
					isCountingUp = false;
				}
			}
			else
			{
				if (m1 > -9)
				{
					m1 -= 0.1;
				}
				else
				{
					isCountingUp = true;
				}
			}
			DAC_1->write(m1);
			DAC_2->write(m1);
#endif
		}
	}

public:
	DAC1_Device *DAC_1;
	DAC2_Device *DAC_2;
	DAC2_Device *DAC_3;
	float m1 = 0;
	float m2 = 0;
};

AOTestModule *module;

// /******************************
//  *         INTERRUPTS          *
//  *******************************/

#ifdef PROBE_SPI
void DMA1_Stream3_IRQHandler(void)
{
	module->DAC_3->dma_transmission_callback();
}
void SPI2_IRQHandler(void)
{
	module->DAC_3->dma_transmission_callback();
}
#endif

void DMA2_Stream3_IRQHandler(void)
{
	module->DAC_2->dma_transmission_callback();
}
void SPI5_IRQHandler(void)
{
	module->DAC_2->dma_transmission_callback();
}

void BDMA_Channel1_IRQHandler(void)
{
	module->DAC_1->dma_transmission_callback();
}

void SPI6_IRQHandler(void)
{
	module->DAC_1->dma_transmission_callback();
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

	module = new AOTestModule();

	module->run();
}
#endif
