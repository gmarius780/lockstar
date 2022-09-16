/*
 * TestModule.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"


#include "../HAL/spi.hpp"
#include "../HAL/rpi.h"
#include "../HAL/leds.hpp"
#include "../Lib/RPIDataPackage.h"

#ifdef TEST_MODULE

class TestModule {
public:
	TestModule() {
		this->rpi = new RPI();
	}
	void run() {

		while(1) {
			turn_LED5_on();
			HAL_Delay(500);
			turn_LED5_off();
			HAL_Delay(500);
		}
	}

	void handle_rpi_input() {
		RPIDataPackage* read_package = rpi->get_read_package();

		switch (read_package->pop_from_buffer<uint32_t>()) {
		case 1:
			method_one(read_package);
			break;
		case 2:
			method_two(read_package);
			break;
		}


		//rpi->send_package(((uint32_t*)rpi->get_read_buffer())[0], ((uint32_t*)rpi->get_read_buffer())[1]);
	}

	void method_one(RPIDataPackage* read_package) {
		RPIDataPackage* write_package = rpi->get_write_package();

		uint32_t p = read_package->pop_from_buffer<uint32_t>();
		uint32_t i = read_package->pop_from_buffer<uint32_t>();

		write_package->push_to_buffer<uint32_t>(p);
		write_package->push_to_buffer<uint32_t>(i);

		rpi->send_package(write_package);
	}

	void method_two(RPIDataPackage* read_package) {
		RPIDataPackage* write_package = rpi->get_write_package();

		read_package->pop_from_buffer<uint32_t>();

		write_package->push_to_buffer<uint32_t>(read_package->pop_from_buffer<uint32_t>());
		write_package->push_to_buffer<uint32_t>(read_package->pop_from_buffer<uint32_t>());

		rpi->send_package(write_package);
	}

	void rpi_dma_in_interrupt() {

		if(rpi->dma_in_interrupt())
		{ /*got new package from rpi*/
			handle_rpi_input();
		} else
		{ /* error */

		}
	}

public:
	RPI *rpi;
};


TestModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

// Interrupt for Digital In line (Trigger)
//ram_func: https://rhye.org/post/stm32-with-opencm3-4-memory-sections/

//__attribute__((section("sram_func")))
//void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
//{
////	module->trigger_interrupt();
//	// Falling Edge = Trigger going High
//	/*if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_RESET){
//		locking = true;
//		PIDLoop->Reset();
//		PIDLoop2->Reset();
//		PIDLoop2->pre_output=2.5;
//		turn_LED6_on();
//
//	}
//	// Rising Edge = Trigger going Low
//	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_SET){
//		locking = false;
//		turn_LED6_off();
//	}*/
//
//}

// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi
__attribute__((section("sram_func")))
__weak void DMA2_Stream4_IRQHandler(void)
{
//	module->dac_2->Callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
//	module->dac_1->Callback();
}
//s __attribute__((section("sram_func")))
//s void DMA2_Stream2_IRQHandler(void)
//s {
	// SPI 1 rx
//	module->adc_dev->Callback();
//	module->adc_interrupt();
//s }
__attribute__((section("sram_func")))
void DMA2_Stream3_IRQHandler(void)
{
	// SPI 1 tx - SPI 5 rx
	// no action required
}
__attribute__((section("sram_func")))
void DMA2_Stream0_IRQHandler(void)
{
	// SPI 4 Rx
	//module->rpi->Callback();
//	module->rpi_dma_interrupt();
	//module->rpi->ResetIntPin();
	//todo: check if error or transmission complete!!
	module->rpi_dma_in_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	// no action required
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

	module = new TestModule();
//	while(1) {
//		HAL_Delay(200);
//	}
	module->run();


}

#endif
