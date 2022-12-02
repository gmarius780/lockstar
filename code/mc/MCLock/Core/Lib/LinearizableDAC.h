/*
 * LinearizableDAC.h
 *
 * Wrapper class of DAC device, which additionally allows for linearization.
 * The reason to put this in a separate class is that linearization is not
 * actually something belonging to the HAL (hardware abstraction layer)
 *
 *  Created on: Dec 2, 2022
 *      Author: marius
 */

#ifndef LIB_LINEARIZABLEDAC_H_
#define LIB_LINEARIZABLEDAC_H_

#include "../HAL/dac_new.hpp"

class LinearizableDAC {
public:
	LinearizableDAC(uint8_t SPI, uint8_t dma_stream_out, uint8_t dma_channel_out, GPIO_TypeDef* sync_port, uint16_t sync_pin, GPIO_TypeDef* clear_port, uint16_t clear_pin) {
		dac = new DAC_Device(SPI, dma_stream_out, dma_channel_out, sync_port, sync_pin, clear_port, clear_pin);

		linearization_available = false;
		linearization_enabled = false;
		linearization_length = 0;
		current_pivot_index = 0;
	}
	virtual ~LinearizableDAC();

	void config_output(ADC_HandleTypeDef* hadc, uint32_t ADC_SENL, uint32_t ADC_SENH) { dac->config_output(hadc, ADC_SENL, ADC_SENH);}

	void write(float value) {
		if (linearization_enabled) {
			dac->write(linearize(value));
		} else {
			dac->write(value);
		}
	}
	void dma_transmission_callback() {dac->dma_transmission_callback();}
	bool is_busy() { return dac->is_busy(); }
	void set_min_output(float m) {
		dac->set_min_output(m);
		ramp_range = dac->get_max_output() - dac->get_min_output();
	}
	void set_max_output(float m) {
		dac->set_max_output(m);
		ramp_range = dac->get_max_output() - dac->get_min_output();
	}
	float get_min_output() {return dac->get_min_output();}
	float get_max_output() {return dac->get_max_output();}

	/**
	 * initializes the buffer which stores the linearization parameters (pivots).
	 * Expects the buffer length as parameter.
	 * Don't call this method unless you set the length for the first time or want to change it
	 * otherwise the system can run out of ram since new simply increments the heap(?) pointer
	 */
	void set_linearization_length(uint32_t length) {
		linearization_length = length;
		linearization_buffer = new float[length]();
	}

	/**
	 * add pivot points to the linearization buffer
	 */
	bool push_to_linearization_buffer(float linearization_pivot, bool append);

	void enable_linearization() {
		if (linearization_available) {
			linearization_enabled = true;
		}
	}
	void disable_linearization() {
		linearization_enabled = false;
	}



private:
	float linearize(float value);

	DAC_Device* dac;

	uint32_t linearization_length;
	uint32_t current_pivot_index;
	float *linearization_buffer;

	bool linearization_available, linearization_enabled;

	float output_range;
	float ramp_range;
	float output_min;
	float output_max;
	float pivot_spacing;
};

#endif /* LIB_LINEARIZABLEDAC_H_ */
