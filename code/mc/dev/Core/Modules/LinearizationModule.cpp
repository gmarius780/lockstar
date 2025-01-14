/*
 * LinearizationModule.cpp
 *
 * Allows the user to linearize the two outputs. The recorded linearization can
 * then be used in all the other modules
 *
 *  Created on: Aug 18, 2022
 *      Author: marius, based on Samuels implementation
 */
#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_it.h"

#include "../HAL/ADCDevice.hpp"

#include "../HAL/BasicTimer.hpp"
#include "../HAL/DACDevice.hpp"
#include "../HAL/leds.hpp"
#include "../HAL/rpi.h"
#include "../HAL/spi.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../Lib/pid.hpp"

#include "../Lib/Module.h"

#ifdef LINEARIZATION_MODULE

etl::circular_buffer<float, 1> aCalculatedSinBuffer;
etl::circular_buffer<float, 1> bCalculatedSinBuffer;

etl::circular_buffer<uint8_t, 1> byteBuffer;

etl::icircular_buffer<float>::iterator itr = aCalculatedSinBuffer.begin();
etl::icircular_buffer<float>::iterator itr2 = bCalculatedSinBuffer.begin();

etl::atomic<bool> unlocked = false;
etl::atomic<bool> unlocked2 = false;

/**
 * State-Machine:
 *
 *  |Idle| --(set_ramp_parameters)--> received_ramp_parameters
 * --(linearize_ch)--> measuring_gain -->finished_gain_measurement | (output_test_ramp)  \																|(get_gain_measurement_result)
 *  		|testing|
 * ------------------------------------------------------------------
 */
enum Channel { CH_ONE, CH_TWO, CH_NOT_SET };
enum State {
  IDLE,
  RECEIVED_RAMP_PARAMETERS,
  RECEIVED_LINEARIZE_COMMAND,
  MEASURING_GAIN,
  FINISHED_GAIN_MEASUREMENT,
  TESTING
};

struct ModuleState {
  Channel active_channel;
  State current_state;
};

class LinearizationModule : public Module {
public:
  LinearizationModule() : Module() {
    initialize_rpi();
    turn_LED6_off();
    turn_LED5_on();

    ramp_start = 0;
    ramp_end = 0;
    ramp_stepsize = 0;
    ramp_length = 0;

    // Initialize state machine
    state.active_channel = CH_NOT_SET;
    state.current_state = IDLE;

    gain_measurement_buffer =
        new float[this->dac_1->MAX_LINEARIZATION_LENGTH]();
  }

  void run() {

    initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
    // this->dac_1->write(0);
    // this->dac_2->write(0);

    /*** work loop ***/
    while (true) {
      HAL_Delay(100);
      adc->start_conversion();
      if (state.current_state == RECEIVED_LINEARIZE_COMMAND and
          state.active_channel != CH_NOT_SET) {
        perform_gain_measurement();
      }
    }
  }

  void handle_rpi_input() {
    if (Module::handle_rpi_base_methods() ==
        false) { // if base class doesn't know the called method
      /*** Package format: method_identifier (uint32_t) | method specific
       * arguments (defined in the methods directly) ***/
      RPIDataPackage *read_package = rpi->get_read_package();

      // switch between method_identifier
      switch (read_package->pop_from_buffer<uint32_t>()) {
      case METHOD_SET_RAMP_PARAMETERS:
        set_ramp_parameters(read_package);
        break;
      case METHOD_LINEARIZE_CH_ONE:
        linearize_ch_one(read_package);
        break;
      case METHOD_LINEARIZE_CH_TWO:
        linearize_ch_two(read_package);
        break;
      case METHOD_GET_GAIN_MEASUREMENT_RESULT:
        get_gain_measurement_result(read_package);
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

  /**
   * Start outputting the ramp on the right channel and recording the trace
   */
  void perform_gain_measurement() {
    if (state.current_state == RECEIVED_LINEARIZE_COMMAND and
        state.active_channel != CH_NOT_SET) {
      //*** Perform gain measurement
      state.current_state = MEASURING_GAIN;

      uint32_t gain_measurement_index = 0;

      LinearizableDAC *dac = state.active_channel == CH_ONE ? dac_1 : dac_2;
      ADC_Device_Channel *adc_channel =
          state.active_channel == CH_ONE ? adc->channel1 : adc->channel2;

      // it doesn't make sense to use the dac outside of the linearized range
      // dac->set_min_output(ramp_start);
      // dac->set_max_output(ramp_end);

      // sequentially perform gain measurement
      while (gain_measurement_index < ramp_length) {
        dac->write(ramp_start + ramp_stepsize * gain_measurement_index);
        // HAL_Delay(
        //     settling_time_ms); // wait for settling time to start conversion
        adc->start_conversion();
        HAL_Delay(1); // wait for conversion to finish (10us would be enough)
        gain_measurement_buffer[gain_measurement_index] =
            adc_channel->get_result();

        gain_measurement_index++;
      }

      state.current_state = FINISHED_GAIN_MEASUREMENT;

      // send ack to signal the RPI that gain mmt is done
      RPIDataPackage *ack = rpi->get_write_package();
      ack->push_ack();
      rpi->send_package(ack);
    }
  }

  /*** START: METHODS ACCESSIBLE FROM THE RPI ***/

  /**
   * Reads ramp parameter AND INSTANTIATES THE BUFFER WITH THE GIVEN RAMP LENGTH
   * this method should therefore not be called too many times as otherwise the
   * RAM will run out. If one wants to perform multiple linearization attempts,
   * one can call the linearize_ch_one/linearize_ch_two method without calling
   * set_ramp_parameters again
   *
   * expects:
   *  - (float) ramp_start : start voltage of ramp
   *  - (float) ramp_end : end voltage of ramp
   *  - (uint32_t) ramp_length: number of points in the ramp
   *  - (uint32_t) settling_time_ms: wait interval between outputting voltage
   * and measuring it
   */
  static const uint32_t METHOD_SET_RAMP_PARAMETERS = 11;
  void set_ramp_parameters(RPIDataPackage *read_package) {
    if (state.current_state == IDLE or
        state.current_state == RECEIVED_RAMP_PARAMETERS or
        state.current_state == FINISHED_GAIN_MEASUREMENT) {
      /***Read arguments***/
      ramp_start = read_package->pop_from_buffer<float>();
      ramp_end = read_package->pop_from_buffer<float>();
      ramp_length = read_package->pop_from_buffer<uint32_t>();
      ramp_range = ramp_end - ramp_start;
      ramp_stepsize = ramp_range / ramp_length;
      settling_time_ms = read_package->pop_from_buffer<uint32_t>();

      if (ramp_length <= this->dac_1->MAX_LINEARIZATION_LENGTH) {
        state.current_state = RECEIVED_RAMP_PARAMETERS;
        /*** send ACK ***/
        RPIDataPackage *write_package = rpi->get_write_package();
        write_package->push_ack();
        rpi->send_package(write_package);
      } else {
        /*** send NACK because the ramp_length is too high***/
        RPIDataPackage *write_package = rpi->get_write_package();
        write_package->push_nack();
        rpi->send_package(write_package);
      }
    } else {
      /*** send NACK because the MC is not ready to read new ramp params***/
      RPIDataPackage *write_package = rpi->get_write_package();
      write_package->push_nack();
      rpi->send_package(write_package);
    }
  }

  /**
   * Starts linearization procedure for channel one
   */
  static const uint32_t METHOD_LINEARIZE_CH_ONE = 12;
  void linearize_ch_one(RPIDataPackage *read_package) {
    linearize_ch(read_package, true);
  }

  /**
   * Starts linearization procedure for channel two
   */
  static const uint32_t METHOD_LINEARIZE_CH_TWO = 13;
  void linearize_ch_two(RPIDataPackage *read_package) {
    linearize_ch(read_package, false);
  }

  void linearize_ch(RPIDataPackage *read_package, bool ch_one) {
    if (state.current_state == RECEIVED_RAMP_PARAMETERS) {
      state.active_channel = ch_one ? CH_ONE : CH_TWO;
      state.current_state = RECEIVED_LINEARIZE_COMMAND;
    } else {
      /*** send NACK because the MC is not ready to perform a new
       * linearization***/
      RPIDataPackage *write_package = rpi->get_write_package();
      write_package->push_nack();
      rpi->send_package(write_package);
    }
  }

  /**
   * Returns the measured gain of the system.
   *
   * expects starting index and number of datapoints to be read as arguments
   */
  static const uint32_t METHOD_GET_GAIN_MEASUREMENT_RESULT = 14;
  void get_gain_measurement_result(RPIDataPackage *read_package) {
    if (state.current_state == FINISHED_GAIN_MEASUREMENT or
        state.current_state == RECEIVED_RAMP_PARAMETERS) {
      // return gain measurement result
      uint32_t buffer_offset = read_package->pop_from_buffer<uint32_t>();
      uint32_t package_size = read_package->pop_from_buffer<uint32_t>();

      if (buffer_offset + package_size > ramp_length) {
        RPIDataPackage *nack = rpi->get_write_package();
        nack->push_nack();
        rpi->send_package(nack);
        return;
      }

      RPIDataPackage *data_package = rpi->get_write_package();
      for (uint32_t i = 0; i < package_size; i++)
        data_package->push_to_buffer<float>(
            gain_measurement_buffer[buffer_offset + i]);
      rpi->send_package(data_package);

      state.current_state = RECEIVED_RAMP_PARAMETERS;
    } else {
      /*** send NACK because there is no gain measurement result available***/
      RPIDataPackage *write_package = rpi->get_write_package();
      write_package->push_nack();
      rpi->send_package(write_package);
    }
  }

  /*** END: METHODS ACCESSIBLE FROM THE RPI ***/

  void rpi_dma_in_interrupt() {

    if (rpi->dma_in_interrupt()) { /*got new package from rpi*/
      handle_rpi_input();
    } else { /* error */
    }
  }

public:
  BasicTimer *timer;

  // ramp parameters
  float ramp_start;
  float ramp_end;
  float ramp_stepsize;
  float ramp_range;
  uint32_t ramp_length;
  uint32_t settling_time_ms;

  float *gain_measurement_buffer;

  ModuleState state;
};

LinearizationModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI
 *communication finished, change on trigger line etc */

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
  // sample = true;
}
__attribute__((section(".itcmram"))) void TIM5_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM5);
  // module->sampling_timer_interrupt();
  // sample2 = true;
}
__attribute__((section(".itcmram"))) void TIM4_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM4);

  // module->rpi->comm_reset_timer_interrupt();
}
__attribute__((section(".itcmram"))) void TIM8_UP_IRQHandler(void) {
  if (TIM8->SR & TIM_SR_UIF) {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    // module->enable_sampling();
    TIM8->SR &= ~TIM_SR_UIF;
  }
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void) {

  /* After power on, give all devices a moment to properly start up */
  HAL_Delay(200);

  module = new LinearizationModule();

  module->run();
}

#endif
