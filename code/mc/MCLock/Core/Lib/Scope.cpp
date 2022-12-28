/*
 * Scope.cpp
 *
 *  Created on: Dec 27, 2022
 *      Author: marius
 */

#include "Scope.h"

Scope::Scope(ADC_Device *adc, LinearizableDAC *dac_1, LinearizableDAC *dac_2) {
	/*
	 * Used to sample the desired values (Input 1/2, output 1/2)
	 * */
	timer = new BasicTimer(7, 1000, 1);
	timer->disable();
	timer->disable_interrupt();
	timer->reset_counter();

	setup = adc_active_mode = sample_in_one = sample_in_two = sample_out_one = sample_out_two = false;
	buffer_length = buffer_index = 0;

	this->adc = adc;
	this->dac_1 = dac_1;
	this->dac_2 = dac_2;
}

Scope::~Scope() {
	// TODO Auto-generated destructor stub
}

bool Scope::setup_scope(uint32_t sampling_prescaler, uint32_t sampling_counter_max, bool sample_in_one, bool sample_in_two, \
		bool sample_out_one, bool sample_out_two, uint32_t buffer_length, bool adc_active_mode) {
	if (buffer_length <= MAX_BUFFER_LENGTH_NBR_OF_FLOATS and buffer_length > 0 and \
			(sample_in_one or sample_in_two or sample_out_one or sample_out_two)) {
		if (setup)
			this->disable();

		this->buffer_length = buffer_length;
		this->sample_in_one = sample_in_one;
		this->sample_in_two = sample_in_two;
		this->sample_out_one = sample_out_one;
		this->sample_out_two = sample_out_two;
		this->adc_active_mode = adc_active_mode;

		buffer_index = 0;


		set_sampling_rate(sampling_prescaler, sampling_counter_max);

		if (sample_in_one) {
			buffer_in_one = new float[buffer_length]();
		}
		if (sample_in_two) {
			buffer_in_two = new float[buffer_length]();
		}
		if (sample_out_one) {
			buffer_out_one = new float[buffer_length]();
		}
		if (sample_out_two) {
			buffer_out_two = new float[buffer_length]();
		}
		setup = true;
		return true;
	} else{
		return false;
	}
}

bool Scope::sample() {
	//if buffer_index >= buffer_length: push_buffers_to_rpi_data_package has to be called first
	if(setup and buffer_index < buffer_length) {
		if (sample_in_one) {
			buffer_in_one[buffer_index] = adc->channel1->get_result();
		}
		if (sample_in_two) {
			buffer_in_two[buffer_index] = adc->channel2->get_result();
		}
		if (sample_out_one) {
			buffer_out_one[buffer_index] = dac_1->get_last_output();
		}
		if (sample_out_two) {
			buffer_out_two[buffer_index] = dac_2->get_last_output();
		}
		buffer_index++;
		return true;
	} else {
		return false;
	}
}

void Scope::timer_interrupt() {
	if (this->setup) {
		this->sample();
		if (adc_active_mode)
			this->adc->start_conversion();
	}
}

bool Scope::push_buffers_to_rpi_data_package(RPIDataPackage* data_package, uint32_t buffer_offset, uint32_t package_size) {
	if(setup and buffer_index > 0 and buffer_offset + package_size <= buffer_length) {
		if (sample_in_one) {
			for(uint32_t i=0; i<package_size; i++)
				if (buffer_offset + i < this->buffer_index)
					data_package->push_to_buffer<float>(buffer_in_one[buffer_offset + i]);
				else
					data_package->push_to_buffer<float>(0.);
		}
		if (sample_in_two) {
			for(uint32_t i=0; i<package_size; i++)
				if (buffer_offset + i < this->buffer_index)
					data_package->push_to_buffer<float>(buffer_in_two[buffer_offset + i]);
				else
					data_package->push_to_buffer<float>(0.);
		}
		if (sample_out_one) {
			for(uint32_t i=0; i<package_size; i++)
				if (buffer_offset + i < this->buffer_index)
					data_package->push_to_buffer<float>(buffer_out_one[buffer_offset + i]);
				else
					data_package->push_to_buffer<float>(0.);
		}
		if (sample_out_two) {
			for(uint32_t i=0; i<package_size; i++)
				if (buffer_offset + i < this->buffer_index)
					data_package->push_to_buffer<float>(buffer_out_two[buffer_offset + i]);
				else
					data_package->push_to_buffer<float>(0.);
		}

		if (buffer_offset + package_size == buffer_length) // the whole buffer has been read
			buffer_index = 0;
		return true;
	} else {
		return false;
	}
}

bool Scope::set_sampling_rate(uint32_t sampling_prescaler, uint32_t sampling_counter_max) {
	timer->set_prescaler(sampling_prescaler);
	timer->set_auto_reload(sampling_counter_max);
	timer->reset_counter();
	return true;
}

bool Scope::enable() {
	if(setup) {
		buffer_index = 0; //reset buffer to initial position
		timer->enable_interrupt();
		timer->enable();
		return true;
	} else {
		return false;
	}
}

bool Scope::disable() {
	if(setup) {
		timer->disable();
		timer->disable_interrupt();
		timer->reset_counter();
		return true;
	} else {
		return false;
	}
}

