/*
 * LinearizableDAC.cpp
 *
 *  Created on: Dec 2, 2022
 *      Author: marius
 */

#include "LinearizableDAC.h"

LinearizableDAC::LinearizableDAC(uint8_t SPI, uint8_t dma_stream_out,
                                 uint8_t dma_channel_out,
                                 GPIO_TypeDef *sync_port, uint16_t sync_pin,
                                 GPIO_TypeDef *clear_port, uint16_t clear_pin) {
  dac = new DAC_Device(SPI, dma_stream_out, dma_channel_out, sync_port,
                       sync_pin, clear_port, clear_pin);

  linearization_available = false;
  linearization_enabled = false;
  linearization_length = 0;
  current_pivot_index = 0;

  linearization_buffer = new float[MAX_LINEARIZATION_LENGTH]();
}

LinearizableDAC::LinearizableDAC(DAC_Device_TypeDef *DAC_conf) {
  if (DAC_conf->isBDMA) {
    dac = new DAC1_Device(DAC_conf);
  } else {
    dac = new DAC2_Device(DAC_conf);
  }

  linearization_available = false;
  linearization_enabled = false;
  linearization_length = 0;
  current_pivot_index = 0;

  linearization_buffer = new float[MAX_LINEARIZATION_LENGTH]();
}

LinearizableDAC::~LinearizableDAC() {
  // TODO Auto-generated destructor stub
}
__attribute__((section(".itcmram"))) void LinearizableDAC::config_output() {
  dac->config_output();
}
__attribute__((section(".itcmram"))) void LinearizableDAC::write(float value) {
  if (linearization_enabled) {
  	dac->write(linearize(value));
  } else {
  dac->write(value);
  }
}
__attribute__((section(".itcmram"))) void LinearizableDAC::write() {
  // if (linearization_enabled) {
  // 	dac->write(linearize(value));
  // } else {
  dac->write();
  // }
}

__attribute__((section(".itcmram"))) void
LinearizableDAC::dma_transmission_callback() {
  dac->dma_transmission_callback();
}

bool LinearizableDAC::is_busy() { return dac->is_busy(); }

void LinearizableDAC::set_min_output(float m) {
  dac->set_min_output(m);
  ramp_range = dac->get_max_output() - dac->get_min_output();
}

void LinearizableDAC::set_max_output(float m) {
  dac->set_max_output(m);
  ramp_range = dac->get_max_output() - dac->get_min_output();
}

void LinearizableDAC::unclamp_output() {
  dac->unclamp_output();
}
void LinearizableDAC::set_clear_state() { dac->set_clear_state(); }

float LinearizableDAC::get_min_output() { return dac->get_min_output(); }

float LinearizableDAC::get_max_output() { return dac->get_max_output(); }

float LinearizableDAC::get_last_output() { return dac->get_last_output(); }
__attribute__((section(".itcmram"))) void
LinearizableDAC::set_linearization_length(uint32_t length) {
  linearization_length = length;
}

void LinearizableDAC::disable_linearization() { linearization_enabled = false; }
__attribute__((section(".itcmram"))) bool
LinearizableDAC::enable_linearization() {
  if (linearization_available) {
    linearization_enabled = true;
    return true;
  } else {
    return false;
  }
}

__attribute__((section(".itcmram"))) float
LinearizableDAC::linearize(float value) {
  value = value / ramp_range * output_range;

  uint32_t pivot_index =
      (uint32_t)((value - output_min) / output_range * linearization_length);
  float pivot = output_min + pivot_spacing * pivot_index;
  float interpolation = (value - pivot) / pivot_spacing;
  if (interpolation < 0)
    interpolation = 0;
  if (pivot_index > linearization_length - 2)
    return linearization_buffer[linearization_length - 1];
  return linearization_buffer[pivot_index] +
         (linearization_buffer[pivot_index + 1] -
          linearization_buffer[pivot_index]) *
             interpolation;
}

__attribute__((section(".itcmram"))) bool
LinearizableDAC::push_to_linearization_buffer(float linearization_pivot,
                                              bool append) {
  if (linearization_length > 0) {
    if (append == false) {
      current_pivot_index = 0;
    }

    if (current_pivot_index >=
        linearization_length) { // linearization array full
      return false;
    }

    linearization_buffer[current_pivot_index] = linearization_pivot;
    current_pivot_index++;

    if (current_pivot_index >=
        linearization_length) { // activate linearization, once all pivot points
                                // have been received
      linearization_available = true;
      linearization_enabled = true;

      output_min = linearization_buffer[0];
      output_max = linearization_buffer[linearization_length - 1];
      output_range = output_max - output_min;
      pivot_spacing = output_range / linearization_length;
    }

    return true;
  } else {
    return false;
  }
}
