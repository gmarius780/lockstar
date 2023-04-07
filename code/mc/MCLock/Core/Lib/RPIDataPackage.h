/*
 * rpi_data_package.h
 *
 *  Created on: Aug 17, 2022
 *      Author: marius
 */

#ifndef LIB_RPIDATAPACKAGE_H_
#define LIB_RPIDATAPACKAGE_H_
#include "stm32f427xx.h"
#include <cstddef>

class RPIDataPackage {
public:
	RPIDataPackage();
	virtual ~RPIDataPackage();

	template <typename T>
	void push_to_buffer(T value) {
		uint8_t const *p = reinterpret_cast<uint8_t *>(&value);
		for (std::size_t j = 0; j < sizeof(value); ++j)
		{
			current_buffer[j] =  p[j];
		}
		current_buffer = current_buffer + sizeof(value);
	}

	void set_buffer(uint8_t *buffer) {
		this->buffer = buffer;
		this->current_buffer = buffer;
	}

	void push_ack(); //ack = 221194(uint32_t)
	void push_nack(); //nack = 999999(uint32_t)

	template <typename T>
	T __attribute__((optimize(0))) pop_from_buffer() { //optimization had to be deactivated for this function as Ofast resultet in a hard-fault
		current_buffer += sizeof(T);
		return ((T*)(current_buffer - sizeof(T)))[0];
	}

	uint32_t nbr_of_bytes_to_send();

private:
	uint8_t *buffer;
	uint8_t *current_buffer;
};

#endif /* LIB_RPIDATAPACKAGE_H_ */
