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

#include "../HAL/DACDevice.hpp"

class LinearizableDAC {
public:
	LinearizableDAC(uint8_t SPI, uint8_t dma_stream_out, uint8_t dma_channel_out, GPIO_TypeDef* sync_port, uint16_t sync_pin, GPIO_TypeDef* clear_port, uint16_t clear_pin);

	virtual ~LinearizableDAC();

	void config_output(ADC_HandleTypeDef* hadc, uint32_t ADC_SENL, uint32_t ADC_SENH);

	void write(float value);

	void dma_transmission_callback();
	bool is_busy();
	void set_min_output(float m);

	void set_max_output(float m);
	float get_min_output();
	float get_max_output();
	float get_last_output();

	/**
	 * initializes the buffer which stores the linearization parameters (pivots).
	 * Expects the buffer length as parameter.
	 * Don't call this method unless you set the length for the first time or want to change it
	 * otherwise the system can run out of ram since new simply increments the heap(?) pointer
	 */
	void set_linearization_length(uint32_t length);

	/**
	 * add pivot points to the linearization buffer
	 */
	bool push_to_linearization_buffer(float linearization_pivot, bool append);

	bool enable_linearization();
	void disable_linearization();

	static uint16_t MAX_LINEARIZATION_LENGTH = 2000; //max length of linearization, used to precreate the buffers

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
