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

#include "../Lib/BufferBaseModule.h"

#ifdef AWG_MODULE

etl::circular_buffer<waveFunction, 100> functions;
etl::circular_buffer<waveFunction, 100> functions2;
etl::circular_buffer<uint32_t, 100> times_buffer;
etl::circular_buffer<uint32_t, 100> times_buffer2;

etl::circular_buffer<float, 2> aCalculatedSinBuffer;
etl::circular_buffer<float, 2> bCalculatedSinBuffer;
etl::icircular_buffer<float>::iterator itr = aCalculatedSinBuffer.begin();
etl::icircular_buffer<float>::iterator itr2 = bCalculatedSinBuffer.begin();

etl::atomic<bool> unlocked = false;
etl::atomic<bool> unlocked2 = false;

etl::atomic<bool> sample = false;
etl::atomic<bool> sample2 = false;

/**
 * User can upload buffers containing the module will then output the voltages
 * defined in the buffers with a sampling-rate, set by the user
 */
class AWGModule : public BufferBaseModule {
  static const uint32_t BUFFER_LIMIT_kBYTES =
      100; // if this is chosen to large (200) there is no warning, the MC
           // simply crashes (hangs in syscalls.c _exit())
  static const uint32_t MAX_NBR_OF_CHUNKS = 100;

public:
  AWGModule() {
    initialize_rpi();
    turn_LED6_off();
    turn_LED5_on();
    // default sampling rate is INTERNAL_CLOCK_FREQUENCY/prescaler * counter_max
    // = 90e6/90*1000 = 1khz
    prescaler = 275;
    counter_max = 1000;
    this->sampling_timer = new BasicTimer(2, counter_max, prescaler);
    this->sampling_timer2 = new BasicTimer(5, counter_max, prescaler);

    // allocate buffer and chunk space
    this->buffer =
        new float[BUFFER_LIMIT_kBYTES *
                  250]; // contains buffer_one and buffer_two sequentially
    this->chunks = new uint32_t[MAX_NBR_OF_CHUNKS]; // contains chuncks_one and
                                                    // chunks_two sequentially
  }

  void run() {
    initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
    this->dac_1->write(0);
    this->dac_2->write(0);

    /*** work loop ***/
    while (true) {
      if (sample) {
        sampling_timer_interrupt();
      }
      if (sample2) {
        sampling_timer_interrupt2();
      }
      // HAL_Delay(100);
      // this->dac_1->write(this->pid->calculate_output(adc->channel1->get_result(),
      // adc->channel2->get_result(), dt));
    }
  }

  void handle_rpi_input() {
    if (handle_rpi_base_methods() ==
        false) { // if base class doesn't know the called method
      /*** Package format: method_identifier (uint32_t) | method specific
       * arguments (defined in the methods directly) ***/
      RPIDataPackage *read_package = this->rpi->get_read_package();
      // switch between method_identifier
      switch (read_package->pop_from_buffer<uint32_t>()) {
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

  void rpi_dma_in_interrupt() {
    if (rpi->dma_in_interrupt()) { /*got new package from rpi*/
      handle_rpi_input();
    } else { /* error */
    }
  }

  void digital_in_rising_edge() {
    if (this->is_output_ttl) {
      this->output_next_chunk();
      this->enable_sampling();
    }
  }

  void digital_in_falling_edge() {
    if (this->is_output_ttl) {
      this->output_next_chunk();
      this->enable_sampling();
    }
  }

  __attribute__((section(".itcmram"))) void sampling_timer_interrupt() {
    if (current_output_one < current_end_chunk_one) {
      this->dac_1->write(*(current_output_one++));
    } else {
      sampling_timer->disable();
      this->output_next_chunk();
    }
  }
  __attribute__((section(".itcmram"))) void sampling_timer_interrupt2() {
    if (current_output_two < current_end_chunk_two) {
      this->dac_2->write(*(current_output_two++));
    } else {
      sampling_timer2->disable();
      this->output_next_chunk();
    }
  }
};

__attribute__((section(".dtcmram"))) AWGModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI
 *communication finished, change on trigger line etc */

void EXTI9_5_IRQHandler(void) {
  if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != 0x00U) {
    /* DataReady Pin Rising */
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);
    // Rising Edge
    if (HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) ==
        GPIO_PIN_RESET) {
      turn_LED3_on();
      module->digital_in_rising_edge();
    }

    // Falling Edge
    if (HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_SET) {
      turn_LED2_on();
      module->digital_in_falling_edge();
    }
  }
  // Note: Tested with square wave input. Rising and falling edge seem to be
  // inverted?
}

// DMA Interrupts. You probably don't want to change these, they are neccessary
// for the low-level communications between MCU, converters and RPi

/********************
||       ADC       ||
********************/
__attribute__((section(".itcmram"))) void DMA1_Stream4_IRQHandler(void) {
  module->adc->dma_receive_callback();
}

__attribute__((section(".itcmram"))) void DMA1_Stream5_IRQHandler(void) {
  module->adc->dma_transmission_callback();
}
/********************
||       RPI       ||
********************/
__attribute__((section(".itcmram"))) void DMA1_Stream0_IRQHandler(void) {
  module->rpi_dma_in_interrupt();
}
__attribute__((section(".itcmram"))) void DMA1_Stream1_IRQHandler(void) {
  module->rpi->dma_out_interrupt();
}
__attribute__((section(".itcmram"))) void SPI1_IRQHandler(void) {
  module->rpi->spi_interrupt();
}
/********************
||      Timer      ||
********************/
__attribute__((section(".itcmram"))) void TIM2_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM2);
  // module->sampling_timer_interrupt();
  sample = true;
}
__attribute__((section(".itcmram"))) void TIM5_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM5);
  // module->sampling_timer_interrupt();
  sample2 = true;
}
__attribute__((section(".itcmram"))) void TIM4_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM4);

  // module->rpi->comm_reset_timer_interrupt();
}
__attribute__((section(".itcmram"))) void TIM8_UP_IRQHandler(void) {
  if (TIM8->SR & TIM_SR_UIF) {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    module->enable_sampling();
    TIM8->SR &= ~TIM_SR_UIF;
  }
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void) {
  /* To speed up the access to functions, that are often called, we store them
   * in the RAM instead of the FLASH memory. RAM is volatile. We therefore need
   * to load the code into RAM at startup time. For background and explanations,
   * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
   * */

  /* After power on, give all devices a moment to properly start up */
  HAL_Delay(200);

  module = new AWGModule();

  module->run();
}

#endif
