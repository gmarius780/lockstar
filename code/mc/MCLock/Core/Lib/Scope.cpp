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
	ready_to_read = ready_to_write = false;

	this->adc = adc;
	this->dac_1 = dac_1;
	this->dac_2 = dac_2;
}

Scope::~Scope() {
}

bool Scope::setup_scope(uint32_t sampling_prescaler, uint32_t sampling_counter_max, bool sample_in_one, bool sample_in_two, \
		bool sample_out_one, bool sample_out_two, uint32_t buffer_length, bool adc_active_mode) {
	return this->setup_scope(sampling_prescaler, sampling_counter_max, sample_in_one, sample_in_two, sample_out_one, \
			sample_out_two, buffer_length, adc_active_mode, false);
}

bool Scope::setup_scope(uint32_t sampling_prescaler, uint32_t sampling_counter_max, bool sample_in_one, bool sample_in_two, \
		bool sample_out_one, bool sample_out_two, uint32_t buffer_length, bool adc_active_mode, bool double_buffer_mode) {
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

			this->double_buffer_mode = double_buffer_mode;

			buffer_index = 0;
			ready_to_read = false; //there is no trace ready to be read (push_buffers_to_rpi_data_package)
			ready_to_write = true; //new values can be recorded (sample)


			set_sampling_rate(sampling_prescaler, sampling_counter_max);

			if (sample_in_one) {
				buffer_in_one_read = new float[buffer_length]();
				if (double_buffer_mode) //initialize another buffer if needed
					buffer_in_one_write = new float[buffer_length]();
				else
					buffer_in_one_write = buffer_in_one_read;

			}
			if (sample_in_two) {
				buffer_in_two_read = new float[buffer_length]();
				if (double_buffer_mode) //initialize another buffer if needed
					buffer_in_two_write = new float[buffer_length]();
				else
					buffer_in_two_write = buffer_in_two_read;
			}
			if (sample_out_one) {
				buffer_out_one_read = new float[buffer_length]();
				if (double_buffer_mode) //initialize another buffer if needed
					buffer_out_one_write = new float[buffer_length]();
				else
					buffer_out_one_write = buffer_out_one_read;
			}
			if (sample_out_two) {
				buffer_out_two_read = new float[buffer_length]();
				if (double_buffer_mode) //initialize another buffer if needed
					buffer_out_two_write = new float[buffer_length]();
				else
					buffer_out_two_write = buffer_out_two_read;
			}
			setup = true;
			return true;
		} else{
			return false;
		}
}

/**
 * Records new values into the write buffer until trace is full
 */
bool Scope::sample() {
	//if buffer_index >= buffer_length: push_buffers_to_rpi_data_package has to be called first
	if(setup) {
		if (buffer_index < buffer_length and ready_to_write) {
			if (adc_active_mode)
				this->adc->start_conversion();
			if (sample_in_one) {
				buffer_in_one_write[buffer_index] = adc->channel1->get_result();
			}
			if (sample_in_two) {
				buffer_in_two_write[buffer_index] = adc->channel2->get_result();
			}
			if (sample_out_one) {
				buffer_out_one_write[buffer_index] = dac_1->get_last_output();
			}
			if (sample_out_two) {
				buffer_out_two_write[buffer_index] = dac_2->get_last_output();
			}
			buffer_index++;

		} else {
			//write buffer is full, has to be read first
			buffer_index = 0;
			ready_to_write = false;
			ready_to_read = true;
			//this->disable();
		}
		return true;
	} else {
		return false;
	}
}

void Scope::timer_interrupt() {
	this->sample();
}

/**
 * Return values from read buffer between [buffer_offset:buffer_offset+package_size]
 */
bool Scope::push_buffers_to_rpi_data_package(RPIDataPackage* data_package, uint32_t buffer_offset, uint32_t package_size) {
	if(setup and buffer_offset + package_size <= buffer_length) {
		if (ready_to_read) { //return only if a complete trace is in the write buffer
			//swap read/write buffers if write buffer is full
			if (double_buffer_mode and not ready_to_write) {
				this->swap_buffers();
				ready_to_write = true; //re-enable writing
			}

			if (sample_in_one) {
				for(uint32_t i=0; i<package_size; i++)
					data_package->push_to_buffer<float>(buffer_in_one_read[buffer_offset + i]);
			}
			if (sample_in_two) {
				for(uint32_t i=0; i<package_size; i++)
					data_package->push_to_buffer<float>(buffer_in_two_read[buffer_offset + i]);
			}
			if (sample_out_one) {
				for(uint32_t i=0; i<package_size; i++)
					data_package->push_to_buffer<float>(buffer_out_one_read[buffer_offset + i]);
			}
			if (sample_out_two) {
				for(uint32_t i=0; i<package_size; i++)
					data_package->push_to_buffer<float>(buffer_out_two_read[buffer_offset + i]);
			}

			if (buffer_offset + package_size == buffer_length) { // the whole buffer has been read
				ready_to_read = false;

				if (not double_buffer_mode) { //in single buffer mode writing can only start now
					ready_to_write = true;
				}
			}
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void Scope::swap_buffers() {
	if (this->double_buffer_mode) {
		float *swap;

		if (sample_in_one) {
			swap = buffer_in_one_read;
			buffer_in_one_read = buffer_in_one_write;
			buffer_in_one_write = swap;
		}

		if (sample_in_two) {
			swap = buffer_in_two_read;
			buffer_in_two_read = buffer_in_two_write;
			buffer_in_two_write = swap;
		}

		if (sample_out_one) {
			swap = buffer_out_one_read;
			buffer_out_one_read = buffer_out_one_write;
			buffer_out_one_write = swap;
		}

		if (sample_out_two) {
			swap = buffer_out_two_read;
			buffer_out_two_read = buffer_out_two_write;
			buffer_out_two_write = swap;
		}
	}
}

bool Scope::set_sampling_rate(uint32_t sampling_prescaler, uint32_t sampling_counter_max) {
	timer->set_prescaler(sampling_prescaler);
	timer->set_auto_reload(sampling_counter_max);
	timer->reset_counter();
	return true;
}

bool Scope::enable() {
	//buffer has to be ready, otherwise it has to be read first
	if(setup and ready_to_write) {
		//buffer_index = 0; //reset buffer to initial position
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

