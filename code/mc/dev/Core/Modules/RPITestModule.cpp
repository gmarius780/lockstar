#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_it.h"

#include "../HAL/leds.hpp"
#include "../HAL/rpi.h"
#ifdef RPI_TEST_MODULE

class RPITestModule {
public:
  RPITestModule() {}

  void run() {
    this->rpi = new RPI();
    turn_LED3_on();
    while (true) {
    }
  }

  void rpi_dma_in_interrupt() {

    if (rpi->dma_in_interrupt()) { /*got new package from rpi*/
                                   // handle_rpi_input();
    } else {                       /* error */
    }
  }

public:
  RPI *rpi;
};

RPITestModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI
 *communication finished, change on trigger line etc */

void DMA1_Stream0_IRQHandler(void) { module->rpi_dma_in_interrupt(); }
void DMA1_Stream1_IRQHandler(void) {
  // SPI 4 Tx
  module->rpi->dma_out_interrupt();
}

void SPI1_IRQHandler(void) { module->rpi->spi_interrupt(); }

// This function is called whenever a timer reaches its period
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM4) {
    module->rpi->comm_reset_timer_interrupt();
  }
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void) {
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

  module = new RPITestModule();

  module->run();
}

#endif
