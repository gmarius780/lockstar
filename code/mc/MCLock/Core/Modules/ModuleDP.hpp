/*
 * ModuleDP.cpp
 *
 *  Created on: Apr 13, 2022
 *      Author: marius
 */


class ModuleDP {
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
};
