/*
 * TestModule.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */
#include "../HAL/spi.hpp"
#include "../HAL/rpi.h"

class TestModule {
public:
	TestModule() {
		this->rpi = new RPI();
	}
	void run() {

	}

	void rpi_dma_interrupt() {
		this->rpi->dma_interrupt();
		//handle rpi input
	}
	void rpi_spi_interrupt() {
		this->rpi->spi_interrupt();
	}

private:
	RPI *rpi;
};

TestModule *module = new TestModule();

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

// Interrupt for Digital In line (Trigger)
//sram_func: https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
__attribute__((section("sram_func")))
void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
//	module->trigger_interrupt();
	// Falling Edge = Trigger going High
	/*if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_RESET){
		locking = true;
		PIDLoop->Reset();
		PIDLoop2->Reset();
		PIDLoop2->pre_output=2.5;
		turn_LED6_on();

	}
	// Rising Edge = Trigger going Low
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_SET){
		locking = false;
		turn_LED6_off();
	}*/

}

// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi
__attribute__((section("sram_func")))
void DMA2_Stream4_IRQHandler(void)
{
//	module->dac_2->Callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
//	module->dac_1->Callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream2_IRQHandler(void)
{
	// SPI 1 rx
//	module->adc_dev->Callback();
//	module->adc_interrupt();
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
	// SPI 4 Rx
	//module->rpi->Callback();
//	module->rpi_dma_interrupt();
	//module->rpi->ResetIntPin();
}
__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	// no action required
}
__attribute__((section("sram_func")))
void DMA2_Stream6_IRQHandler(void)
{
	// SPI 6 Rx
	// no action required
}

__attribute__((section("sram_func")))
void SPI4_IRQHandler(void) {
	module->rpi_spi_interrupt();
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

	module->run();

}
