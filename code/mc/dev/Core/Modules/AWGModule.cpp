/*
 * AWGModule.cpp
 *
 *
 *  Created on: Aug 18, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"

#include "../HAL/leds.hpp"

#include "BufferBaseModule.h"

#ifdef AWG_MODULE

/**
 * User can upload buffers containing the module will then output the voltages defined in the buffers with a sampling-rate, set by the user
 */
class AWGModule : public BufferBaseModule
{
	static const uint32_t BUFFER_LIMIT_kBYTES = 160; // if this is chosen to large (200) there is no warning, the MC simply crashes (hangs in syscalls.c _exit())
	static const uint32_t MAX_NBR_OF_CHUNKS = 100;

public:
	AWGModule()
	{
		initialize_rpi();
		turn_LED6_off();
		turn_LED5_on();
		// default sampling rate is INTERNAL_CLOCK_FREQUENCY/prescaler * counter_max = 90e6/90*1000 = 1khz
		prescaler = 275;
		counter_max = 1000;
		this->sampling_timer = new BasicTimer(2, counter_max, prescaler);

		// allocate buffer and chunk space
		this->buffer = new float[BUFFER_LIMIT_kBYTES * 250]; // contains buffer_one and buffer_two sequentially
		this->chunks = new uint32_t[MAX_NBR_OF_CHUNKS];		 // contains chuncks_one and chunks_two sequentially
	}

	void run()
	{
		initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
		this->dac_1->write(0);
		this->dac_2->write(0);

		/*** work loop ***/
		while (true)
		{
			// HAL_Delay(100);
			// this->dac_1->write(this->pid->calculate_output(adc->channel1->get_result(), adc->channel2->get_result(), dt));
		}
	}

	void handle_rpi_input()
	{
		if (handle_rpi_base_methods() == false)
		{ // if base class doesn't know the called method
			/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
			RPIDataPackage *read_package = this->rpi->get_read_package();
			// switch between method_identifier
			switch (read_package->pop_from_buffer<uint32_t>())
			{
			default:
				/*** send NACK because the method_identifier is not valid ***/
				RPIDataPackage *write_package = rpi->get_write_package();
				write_package->push_nack();
				rpi->send_package(write_package);
				break;
			}
		}
	}

	/*** START: METHODS ACCESSIBLE FROM THE RPI ***/

	/*** END: METHODS ACCESSIBLE FROM THE RPI ***/

	void rpi_dma_in_interrupt()
	{
		if (rpi->dma_in_interrupt())
		{ /*got new package from rpi*/
			handle_rpi_input();
		}
		else
		{ /* error */
		}
	}

	void digital_in_rising_edge()
	{
		if (this->is_output_ttl)
		{
			this->output_next_chunk();
			this->enable_sampling();
		}
	}

	void digital_in_falling_edge()
	{
		if (this->is_output_ttl)
		{
			this->output_next_chunk();
			this->enable_sampling();
		}
	}

	//__attribute__((section("sram_func")))
	void sampling_timer_interrupt()
	{
		if (current_output_one < current_end_chunk_one)
		{
			this->dac_1->write(*(current_output_one++));
		}
		else
		{
			if (current_output_two >= current_end_chunk_two)
			{
				turn_LED6_off();
				this->disable_sampling();
				this->is_output_on = true;
				this->is_output_ttl = false;
				turn_LED6_on();

				this->output_next_chunk();
			}
		}

		if (current_output_two < current_end_chunk_two)
		{
			this->dac_2->write(*(current_output_two++));
		}
		else
		{
			if (current_output_one >= current_end_chunk_one)
			{
				turn_LED6_off();
				this->disable_sampling();
				this->is_output_on = true;
				this->is_output_ttl = false;
				turn_LED6_on();

				this->output_next_chunk();
			}
		}
	}
};

AWGModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

void EXTI9_5_IRQHandler(void)
{
	if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != 0x00U)
	{
		/* DataReady Pin Rising */
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);
		// Rising Edge
		if (HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_RESET)
		{
			turn_LED3_on();
			module->digital_in_rising_edge();
		}

		// Falling Edge
		if (HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_SET)
		{
			turn_LED2_on();
			module->digital_in_falling_edge();
		}
	}
	// Note: Tested with square wave input. Rising and falling edge seem to be inverted?
}

// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi

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
	module->adc->dma_receive_callback();
}

void DMA1_Stream5_IRQHandler(void)
{
	module->adc->dma_transmission_callback();
}
/********************
||       RPI       ||
********************/
void DMA1_Stream0_IRQHandler(void)
{
	module->rpi_dma_in_interrupt();
}
void DMA1_Stream1_IRQHandler(void)
{
	module->rpi->dma_out_interrupt();
}
void SPI1_IRQHandler(void)
{
	module->rpi->spi_interrupt();
}

void TIM2_IRQHandler(void)
{
	LL_TIM_ClearFlag_UPDATE(TIM2);
	module->sampling_timer_interrupt();
}
void TIM4_IRQHandler(void)
{
	module->rpi->comm_reset_timer_interrupt();
}

void TIM7_IRQHandler(void)
{
	module->scope_timer_interrupt();
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

	module = new AWGModule();

	module->run();
}

#endif
