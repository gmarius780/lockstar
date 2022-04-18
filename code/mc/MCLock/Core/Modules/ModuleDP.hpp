/*
 * ModuleDP.cpp
 *
 *  Created on: Apr 13, 2022
 *      Author: marius
 */
#include <stdint.h>
#include <iosfwd>
#include <cstddef>
#include <cstring>
#include "../HAL/HardwareComponents.hpp"

class ModuleDP {
	/* DataPackage structure for rpi-calls to the MC:
	 * uint8_t (8bit): method identifier | sequence of arguments sent to the called methods (same order as defined in the function definition in the Module class)
	 *
	 * DataPackage structure for MC response to rpi:
	 *
	 * For ACK/NACK: Two booleans: when both true, this corresponds to an ACK
	 * For general response: unsigned int () corresponding to payload length, <payload>
	 */
public:
	template <typename T>
	static uint8_t * push_to_buffer(uint8_t *buffer, T value) {
		uint8_t const *p = reinterpret_cast<uint8_t *>(&value);
		for (std::size_t j = 0; j < sizeof(T); ++j)
		{
			buffer[j] =  p[j];
		}
		return buffer + sizeof(T);
	}

	template <typename T>
	static uint8_t * pop_from_buffer(uint8_t *buffer, T *value) {
		memcpy(value, buffer - sizeof(T), sizeof(T));
		return buffer - sizeof(T);
	}

	static uint8_t * pop_method_identifier(uint8_t *buffer, uint8_t *value) {
		return ModuleDP::pop_from_buffer<uint8_t>(buffer, value);
	}
};
