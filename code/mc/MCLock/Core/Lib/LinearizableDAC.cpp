/*
 * LinearizableDAC.cpp
 *
 *  Created on: Dec 2, 2022
 *      Author: marius
 */

#include "LinearizableDAC.h"


LinearizableDAC::~LinearizableDAC() {
	// TODO Auto-generated destructor stub
}

float LinearizableDAC::linearize(float value) {
	value = value/ramp_range*output_range;

	uint32_t pivot_index = (uint32_t)((value-output_min)/output_range*linearization_length);
	float pivot = output_min + pivot_spacing*pivot_index;
	float interpolation = (value-pivot)/pivot_spacing;
	if(interpolation < 0)
		interpolation = 0;
	if(pivot_index > linearization_length-2)
		return linearization_buffer[pivot_index];
	return linearization_buffer[pivot_index] + (linearization_buffer[pivot_index+1]-linearization_buffer[pivot_index])*interpolation;
}

bool LinearizableDAC::push_to_linearization_buffer(float linearization_pivot, bool append) {
	if (linearization_length > 0) {
		if (append == false) {
			current_pivot_index = 0;
		}

		if (current_pivot_index >= linearization_length) { //linearization array full
			return false;
		}

		linearization_buffer[current_pivot_index] = linearization_pivot;
		current_pivot_index ++;

		if (current_pivot_index >= linearization_length) { //activate linearization, once all pivot points have been received
			linearization_available = true;
			linearization_enabled = true;

			output_min = linearization_buffer[0];
			output_max = linearization_buffer[linearization_length-1];
			output_range = output_max - output_min;
			pivot_spacing = output_range/linearization_length;
		}

		return true;
	} else {
		return false;
	}
}
