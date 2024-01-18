/*
 * LedModule.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */
#include "main.h"

#include "../HAL/leds.hpp"

#ifdef LED_MODULE

class LedModule {
public:
  LedModule() {}
  void run() {

    while (1) {
      turn_LED1_on();
      turn_LED2_on();
      turn_LED3_on();
      turn_LED4_on();
      turn_LED5_on();
      HAL_Delay(500);
      turn_LED5_off();
      HAL_Delay(500);
    }
  }

public:
};

LedModule *module;

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void) {
  /* To speed up the access to functions, that are often called, we store them
   * in the RAM instead of the FLASH memory. RAM is volatile. We therefore need
   * to load the code into RAM at startup time. For background and explanations,
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

  module = new LedModule();
  //	while(1) {
  //		HAL_Delay(200);
  //	}
  module->run();
}

#endif
