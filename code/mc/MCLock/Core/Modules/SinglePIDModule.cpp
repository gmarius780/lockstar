/*
 * SinglePIDModule.cpp
 *
 *  Implements a PID controller with
 *  Setpoint: INPUT 1
 *  Errorsignal: INPUT 2
 *  Control-output: OUTPUT 1
 *
 *  Created on: Aug 18, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"

#include "../HAL/ADCDevice.hpp"

#include "../HAL/spi.hpp"
#include "../HAL/rpi.h"
#include "../HAL/leds.hpp"
#include "../HAL/BasicTimer.hpp"
#include "../HAL/DACDevice.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../Lib/pid.hpp"

#include "Module.hpp"

#ifdef SINGLE_PID_MODULE



class SinglePIDModule: public Module {
public:
	SinglePIDModule() : Module() {
		initialize_rpi();
		locked = false;
		turn_LED6_off();
		turn_LED5_on();
		pid = new PID(0, 0, 0, 0, 0);
		p = i = d = input_offset = output_offset = 0;
	}

	void run() {
		initialize_adc(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
		initialize_dac();
		this->dac_1->write(0);
		this->dac_2->write(0);

		/*** TIMER FOR MAINLOOP TO EXTRACT DT ***/

		const uint16_t psc = 68;
		const float TIM3freq = 90e6;
		BasicTimer* timer = new BasicTimer(3, 0xFFFF, psc);
		timer->enable();

		float dt = 0;
		uint16_t t = 0;

		/*** work loop ***/
		while(true){
			HAL_Delay(100);
			while(this->locked == true) {
				// Measuring elapsed time per work loop
				t = timer->get_counter() - t;
				dt = t/TIM3freq*psc;
				t = timer->get_counter();

				this->adc->start_conversion();
				this->dac_1->write(this->pid->calculate_output(adc->channel2->get_result(), adc->channel1->get_result(), dt));
			}
		}
	}

	void handle_rpi_input() {
		if (Module::handle_rpi_base_methods() == false) { //if base class doesn't know the called method
			/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
			RPIDataPackage* read_package = rpi->get_read_package();

			// switch between method_identifier
			switch (read_package->pop_from_buffer<uint32_t>()) {
			case METHOD_SET_PID:
				set_pid(read_package);
				break;
			case METHOD_LOCK:
				lock(read_package);
				break;
			case METHOD_UNLOCK:
				unlock(read_package);
				break;
			case METHOD_SET_OUTPUT_LIMITS:
				set_output_limits(read_package);
				break;
			default:
				/*** send NACK because the method_identifier is not valid ***/
				RPIDataPackage* write_package = rpi->get_write_package();
				write_package->push_nack();
				rpi->send_package(write_package);
				break;
			}
		}
	}

	/*** START: METHODS ACCESSIBLE FROM THE RPI ***/
	static const uint32_t METHOD_SET_PID = 11;
	void set_pid(RPIDataPackage* read_package) {
		/***Read arguments***/
		p = read_package->pop_from_buffer<float>();
		i = read_package->pop_from_buffer<float>();
		d = read_package->pop_from_buffer<float>();
		input_offset = read_package->pop_from_buffer<float>();
		output_offset = read_package->pop_from_buffer<float>();

		this->pid->set_pid(p, i, d, input_offset, output_offset);

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_LOCK = 12;
	void lock(RPIDataPackage* read_package) {
		this->locked = true;
		turn_LED6_on();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_UNLOCK = 13;
	void unlock(RPIDataPackage* read_package) {
		this->locked = false;
		this->dac_1->write(0);
		turn_LED6_off();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_OUTPUT_LIMITS = 14;
	void set_output_limits(RPIDataPackage* read_package) {
		set_ch_output_limit(read_package, this->dac_1);
	}

	/*** END: METHODS ACCESSIBLE FROM THE RPI ***/

	void rpi_dma_in_interrupt() {

		if(rpi->dma_in_interrupt())
		{ /*got new package from rpi*/
			handle_rpi_input();
		} else
		{ /* error */

		}
	}

public:
	float p, i, d, input_offset, output_offset;
	PID* pid;
	bool locked;
};


SinglePIDModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */



// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi
__attribute__((section("sram_func")))
__weak void DMA2_Stream4_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream2_IRQHandler(void)
{
	// SPI 1 rx
	module->adc->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream3_IRQHandler(void)
{
	// SPI 1 tx - SPI 5 rx
	// no action required
}
__attribute__((section("sram_func")))
void DMA2_Stream0_IRQHandler(void)
{
	module->rpi_dma_in_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	module->rpi->dma_out_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream6_IRQHandler(void)
{
	// SPI 6 Rx
	// no action required
}

__attribute__((section("sram_func")))
void SPI4_IRQHandler(void) {
	module->rpi->spi_interrupt();
}

__attribute__((section("sram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM4) {
		module->rpi->comm_reset_timer_interrupt();
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
	while (dest < &__sram_func_end) {
	  *dest = *src;
	  src++;
	  dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new SinglePIDModule();

	module->run();

}

#endif
