#include <main.h>
//#include <stm32f427xx.h>
//#include <sys/_stdint.h>
//#include <cstring>
//#include "stm32f4xx_it.h"
//#include "stm32f4xx_hal_gpio.h"
////HAL INCLUDES
//#include "../HAL/spi.hpp"
//#include "../HAL/rpi.h"
//#include "../HAL/leds.hpp"
//#include "../Inc/misc_func.hpp"


#define TEST_MODULE

#ifdef TEST_MODULE
#include "../Modules/TestModule.h"
#endif



void cppmain(void) {
	start();

}


//#define TEST_MODULE
//
//#ifdef TEST_MODULE
//
//#include "../Modules/TestModule.cpp"
//
//start();
//#endif



/******************************
 *       MAIN FUNCTION        *
 ******************************/
//void cppmain(void)
//{
//	/* To speed up the access to functions, that are often called, we store them in the RAM instead of the FLASH memory.
//	 * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
//	 * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
//	 * */
//	extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
//	volatile unsigned *src = &__sram_func_loadaddr;
//	volatile unsigned *dest = &__sram_func_start;
//	while (dest < &__sram_func_end) {
//	  *dest = *src;
//	  src++;
//	  dest++;
//	}
//
//	/* After power on, give all devices a moment to properly start up */
//	HAL_Delay(200);
//
//	module->run();
//
//}
