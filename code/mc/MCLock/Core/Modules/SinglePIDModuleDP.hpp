/*
 * SinglePIDModuleDP.cpp
 *
 *  Created on: Apr 13, 2022
 *      Author: marius
 */

#include "ModuleDP.hpp"

enum SinglePIDMethods {
	initialize = (uint8_t)0,
	set_pid = (uint8_t)1
};

class SinglePIDModuleDP: public ModuleDP {

	static unsigned int size_initialize() {
		return sizeof(uint8_t) + 5 * sizeof(float) + 2 * sizeof(bool) + 3 * sizeof(HardwareComponents);
	}

	static void write_initialize(uint8_t* write_buffer, float p, float i, float d, float out_range_min, float out_range_max, bool useTTL,
			bool locked, HardwareComponents err_channel, HardwareComponents setpoint_channel, HardwareComponents out_channel) {
		uint8_t *current_buffer;
		current_buffer = ModuleDP::push_to_buffer<SinglePIDMethods>(write_buffer, initialize);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, p);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, i);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, d);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, out_range_min);
		current_buffer = ModuleDP::push_to_buffer<float>(current_buffer, out_range_max);
		current_buffer = ModuleDP::push_to_buffer<bool>(current_buffer, useTTL);
		current_buffer = ModuleDP::push_to_buffer<bool>(current_buffer, locked);
		current_buffer = ModuleDP::push_to_buffer<HardwareComponents>(current_buffer, err_channel);
		current_buffer = ModuleDP::push_to_buffer<HardwareComponents>(current_buffer, setpoint_channel);
		current_buffer = ModuleDP::push_to_buffer<HardwareComponents>(current_buffer, out_channel);
	}

	static std::tuple<float, float, float, float, float, bool, bool, HardwareComponents, HardwareComponents, HardwareComponents> read_initialize(uint8_t* read_buffer) {
		float p, i, d, out_range_min, out_range_max;
		bool useTTL, locked;
		HardwareComponents err_channel, setpoint_channel, out_channel;

		uint8_t *current_buffer;
		current_buffer = ModuleDP::pop_from_buffer<float>(read_buffer, &p);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &i);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &d);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &out_range_min);
		current_buffer = ModuleDP::pop_from_buffer<float>(current_buffer, &out_range_max);
		current_buffer = ModuleDP::pop_from_buffer<bool>(current_buffer, &useTTL);
		current_buffer = ModuleDP::pop_from_buffer<bool>(current_buffer, &locked);
		current_buffer = ModuleDP::pop_from_buffer<HardwareComponents>(current_buffer, &err_channel);
		current_buffer = ModuleDP::pop_from_buffer<HardwareComponents>(current_buffer, &setpoint_channel);
		current_buffer = ModuleDP::pop_from_buffer<HardwareComponents>(current_buffer, &out_channel);

		return std::make_tuple(p, i, d, out_range_min, out_range_max, useTTL, locked, err_channel, setpoint_channel, out_channel);
	}

};
