/*
 * SPITestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32h725xx.h"
#include "../HAL/spi.hpp"
#include "../HAL/leds.hpp"

#ifdef SPI_TEST_MODULE_HAL
#pragma message( "Compiling SPI Test Module HAL")  

extern SPI_HandleTypeDef hspi4;

class SPITestModule {
public:
	SPITestModule() {

	}

	void run() {
		spi_number = 4;
        TXbuffer[0] = 0x0F;
        TXbuffer[4] = 0xFF;
        turn_LED3_on();

        SPI_Dev = new SPI(spi_number);
        //__HAL_SPI_ENABLE(hspi4);
        // SPI_Dev->enableSPI();
		// SPI_Dev->enableMasterTransmit();
		while(true) {

			HAL_SPI_TransmitReceive(&hspi4, TXbuffer, RXbuffer, 6, 1000);

		}
	}


public:
	SPI *SPI_Dev;
	uint8_t spi_number;
    uint8_t *TXbuffer = new uint8_t[6]();
    uint8_t *RXbuffer = new uint8_t[6]();
};


SPITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/


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

	module = new SPITestModule();

	module->run();


}
#endif
