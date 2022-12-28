/*
 * Scope.h
 *
 *  Created on: Dec 27, 2022
 *      Author: marius
 */

#ifndef LIB_SCOPE_H_
#define LIB_SCOPE_H_
#include "stm32f4xx_hal.h"
#include "RPIDataPackage.h"
#include "../HAL/BasicTimer.hpp"
#include "LinearizableDAC.h"
#include "../HAL/ADCDevice.hpp"

class Scope {
public:
	static const uint32_t MAX_BUFFER_LENGTH_NBR_OF_FLOATS = 10000;

	Scope(ADC_Device *adc, LinearizableDAC *dac_1, LinearizableDAC *dac_2);
	virtual ~Scope();

	/**
	 * Sets the scope parameters. if it returns true, the scope can be abled using enable().
	 *
	 * Params:
	 *
	 * sampling_prescaler, sampling_counter_max - the timer parameters defining the sampling rate
	 *
	 * sample_<in/out>_<one/two> specify wheter to record the corresponding trace
	 * buffer_length: the length of the buffer that is created for each of the sample_<in/out>_<one/two> you set to true
	 */
	bool setup_scope(uint32_t sampling_prescaler, uint32_t sampling_counter_max, bool sample_in_one, bool sample_in_two, \
			bool sample_out_one, bool sample_out_two, uint32_t buffer_length, bool adc_active_mode);
	bool set_sampling_rate(uint32_t sampling_prescaler, uint32_t sampling_counter_max);
	bool enable();
	bool disable();
	/**
	 * saves the current value into the corresponding buffer for each enabled sample from setup_scope
	 *
	 * if the buffer is full: nothing is done. you have to call push_buffers_to_rpi_data_package first
	 */
	bool sample();
	/*
	 * calls sample and adc->start_conversion if adc_active_mode==true
	 */
	void timer_interrupt();
	/**
	 * Pushes the buffers to the datapackage according to the setup_scope parameters.
	 *
	 */
	bool push_buffers_to_rpi_data_package(RPIDataPackage* data_package, uint32_t buffer_offset, uint32_t package_size);
private:
	bool setup; //whether setup_scope has been called successfully
	bool adc_active_mode; //whether or not scope should call adc->star_conversion()
	bool sample_in_one, sample_in_two, sample_out_one, sample_out_two;
	uint32_t buffer_length;
	float *buffer_in_one, *buffer_in_two, *buffer_out_one, *buffer_out_two;
	uint32_t buffer_index;

	BasicTimer *timer;
	ADC_Device *adc;
	LinearizableDAC *dac_1, *dac_2;
};

#endif /* LIB_SCOPE_H_ */
