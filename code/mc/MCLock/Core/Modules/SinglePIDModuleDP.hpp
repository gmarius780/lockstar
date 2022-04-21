/*
 * SinglePIDModuleDP.hpp
 *
 *  Created on: Apr 21, 2022
 *      Author: marius
 */

#ifndef MODULES_SINGLEPIDMODULEDP_HPP_
#define MODULES_SINGLEPIDMODULEDP_HPP_

#include "../HAL/HardwareComponents.hpp"
#include "ModuleDP.hpp"
#include <tuple>
#include <vector>
#include <stdint.h>

enum struct SinglePIDMethods: uint8_t {
	INITIALIZE = (uint8_t)0,
	SET_PID = (uint8_t)1
};

class SinglePIDModuleDP: public ModuleDP {
public:
	static unsigned int size_initialize_call() {
		// Method + 5* float + 2* bool + 3 * hardwarecompontents
		return sizeof(SinglePIDMethods) + 5 * sizeof(float) + 2 * sizeof(bool) + 3 * sizeof(HardwareComponents);
	}

	static std::vector<uint8_t> write_initialize_call(float p, float i, float d, float out_range_min, float out_range_max, bool useTTL,
			bool locked, HardwareComponents err_channel, HardwareComponents setpoint_channel, HardwareComponents out_channel) {
		uint8_t *write_buffer = new uint8_t[size_initialize_call()];
		uint8_t *current_buffer;
		current_buffer = ModuleDP::push_to_buffer<float>(write_buffer, p);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, i);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, d);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, out_range_min);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, out_range_max);
		current_buffer = ModuleDP::push_to_buffer<bool>(current_buffer, useTTL);
		current_buffer = ModuleDP::push_to_buffer<bool>(current_buffer, locked);
		current_buffer = ModuleDP::push_to_buffer<HardwareComponents>(current_buffer, err_channel);
		current_buffer = ModuleDP::push_to_buffer<HardwareComponents>(current_buffer, setpoint_channel);
		current_buffer = ModuleDP::push_to_buffer<HardwareComponents>(current_buffer, out_channel);

		current_buffer = ModuleDP::push_to_buffer<SinglePIDMethods>(current_buffer, SinglePIDMethods::INITIALIZE);
		std::vector<uint8_t> return_vector(write_buffer, current_buffer);
		return return_vector;
	}

	static std::tuple<float, float, float, float, float, bool, bool, HardwareComponents, HardwareComponents, HardwareComponents> read_initialize_call(uint8_t* read_buffer) {
		float p = 0., i = 0., d = 0., out_range_min = 0., out_range_max = 0.;
		bool useTTL = false, locked = false;
		HardwareComponents err_channel = HardwareComponents::analog_in_one, setpoint_channel = HardwareComponents::analog_in_one, out_channel = HardwareComponents::analog_in_one;

		uint8_t *current_buffer;
		current_buffer = ModuleDP::pop_from_buffer<HardwareComponents>(read_buffer, &out_channel);
		current_buffer = ModuleDP::pop_from_buffer<HardwareComponents>(current_buffer, &setpoint_channel);
		current_buffer = ModuleDP::pop_from_buffer<HardwareComponents>(current_buffer, &err_channel);
		current_buffer = ModuleDP::pop_from_buffer<bool>(current_buffer, &locked);
		current_buffer = ModuleDP::pop_from_buffer<bool>(current_buffer, &useTTL);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &out_range_max);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &out_range_min);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &d);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &i);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &p);

		return std::make_tuple(p, i, d, out_range_min, out_range_max, useTTL, locked, err_channel, setpoint_channel, out_channel);
	}

};


#endif /* MODULES_SINGLEPIDMODULEDP_HPP_ */
