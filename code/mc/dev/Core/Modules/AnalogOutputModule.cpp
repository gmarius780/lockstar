/*
 * AnalogOutputModule.cpp
 *
 *  Created on: Aug 18, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"

#include "../HAL/ADCDevice.hpp"

#include "../HAL/DACDevice.hpp"
#include "../HAL/leds.hpp"
#include "../HAL/rpi.h"
#include "../HAL/spi.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../Lib/pid.hpp"

#include "../Lib/ScopeModule.h"

#ifdef ANALOG_OUTPUT_MODULE
#include "dac_config.h"
#include "etl/circular_buffer.h"
#include <algorithm>
#include "../Lib/BufferBaseModule.h"
/* Array of calculated sines in Q1.31 format */
etl::circular_buffer<float, 100> aCalculatedSinBuffer;
etl::circular_buffer<float, 100> bCalculatedSinBuffer;

etl::circular_buffer<uint8_t, 1000> byteBuffer;

etl::icircular_buffer<float>::iterator itr = aCalculatedSinBuffer.begin();
etl::icircular_buffer<float>::iterator itr2 = bCalculatedSinBuffer.begin();

auto end = aCalculatedSinBuffer.begin();
auto end2 = bCalculatedSinBuffer.begin();

__attribute((section(".dtcmram"))) uint16_t chunk_counter = 0;
__attribute((section(".dtcmram"))) uint16_t current_period = 1;
__attribute((section(".dtcmram"))) uint16_t current_period2 = 1;

etl::circular_buffer<waveFunction, 100> functions;
etl::circular_buffer<waveFunction, 100> functions2;
etl::circular_buffer<uint32_t, 100> times_buffer;
etl::circular_buffer<uint32_t, 100> times_buffer2;

uint32_t count = 0;

etl::atomic<bool> unlocked = false;
etl::atomic<bool> unlocked2 = false;
etl::atomic<bool> sample = false;
etl::atomic<bool> sample2 = false;
etl::atomic<bool> idle = false;
etl::atomic<bool> idle2 = false;
std::atomic_flag lock = ATOMIC_FLAG_INIT;

class AnalogOutputModule : public ScopeModule {
public:
  AnalogOutputModule() : ScopeModule() {
    initialize_rpi();
    turn_LED6_off();
    turn_LED4_on();
    output_value_one = 5;
    output_value_two = 5;
  }

  void run() {
    initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
    this->dac_1->write(0);
    this->dac_2->write(0);

    /*** work loop ***/
    while (true) {
      // HAL_Delay(50);
    }
  }

  void handle_rpi_input() {
    if (ScopeModule::handle_rpi_base_methods() ==
        false) { // if base class doesn't know the called method
      /*** Package format: method_identifier (uint32_t) | method specific
       * arguments (defined in the methods directly) ***/
      RPIDataPackage *read_package = rpi->get_read_package();

      // switch between method_identifier
      switch (read_package->pop_from_buffer<uint32_t>()) {
      case METHOD_OUTPUT_ON:
        output_on(read_package);
        break;
      case METHOD_OUTPUT_OFF:
        output_off(read_package);
        break;
      case METHOD_OUTPUT_TTL:
        output_ttl(read_package);
        break;
      case METHOD_SET_CH_ONE_OUTPUT:
        set_ch_one_output(read_package);
        break;
      case METHOD_SET_CH_TWO_OUTPUT:
        set_ch_two_output(read_package);
        break;
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
  static const uint32_t METHOD_OUTPUT_ON = 11;
  void output_on(RPIDataPackage *read_package) {
    this->is_output_on = true;
    this->is_output_ttl = false;
    this->dac_1->write(this->output_value_one);
    this->dac_2->write(this->output_value_two);
    turn_LED6_on();

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }

  static const uint32_t METHOD_OUTPUT_OFF = 12;
  void output_off(RPIDataPackage *read_package) {
    this->is_output_on = false;
    this->is_output_ttl = false;
    this->dac_1->write(0);
    this->dac_2->write(0);
    turn_LED6_off();

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }

  static const uint32_t METHOD_OUTPUT_TTL = 13;
  void output_ttl(RPIDataPackage *read_package) {
    this->is_output_on = false;
    this->is_output_ttl = true;
    turn_LED6_off();

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }

  static const uint32_t METHOD_SET_CH_ONE_OUTPUT = 16;
  void set_ch_one_output(RPIDataPackage *read_package) {
    /***Read arguments***/
    output_value_one = read_package->pop_from_buffer<float>();
    if (this->is_output_on)
      this->dac_1->write(output_value_one);

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }

  static const uint32_t METHOD_SET_CH_TWO_OUTPUT = 17;
  void set_ch_two_output(RPIDataPackage *read_package) {
    /***Read arguments***/
    output_value_two = read_package->pop_from_buffer<float>();
    if (this->is_output_on)
      this->dac_2->write(output_value_two);

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }

  /*** END: METHODS ACCESSIBLE FROM THE RPI ***/

  void rpi_dma_in_interrupt() {

    if (rpi->dma_in_interrupt()) { /*got new package from rpi*/
      handle_rpi_input();
    } else { /* error */
    }
  }

  void digital_in_rising_edge() {
    if (this->is_output_ttl) {
      this->dac_1->write(this->output_value_one);
      this->dac_2->write(this->output_value_two);
      turn_LED6_on();
    }
  }

  void digital_in_falling_edge() {
    if (this->is_output_ttl) {
      this->dac_1->write(0);
      this->dac_2->write(0);
      turn_LED6_off();
    }
  }

public:
  bool is_output_on, is_output_ttl;
  float output_value_one, output_value_two;
};

AnalogOutputModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI
 *communication finished, change on trigger line etc */

void EXTI9_5_IRQHandler(uint16_t gpio_pin) {
  if (gpio_pin == DigitalIn_Pin) {
    // Rising Edge
    if (HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_RESET)
      module->digital_in_rising_edge();

    // Falling Edge
    if (HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_SET)
      module->digital_in_falling_edge();
  }

  // Note: Tested with square wave input. Rising and falling edge seem to be
  // inverted?
}

// DMA Interrupts. You probably don't want to change these, they are neccessary
// for the low-level communications between MCU, converters and RPi

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

__attribute__((section(".itcmram"))) void TIM4_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM4);

  // module->rpi->comm_reset_timer_interrupt();
}
/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void) {

  /* After power on, give all devices a moment to properly start up */
  HAL_Delay(200);

  module = new AnalogOutputModule();

  module->run();
}

#endif
