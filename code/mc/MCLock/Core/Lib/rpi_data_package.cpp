/*
 * rpi_data_package.cpp
 *
 *  Created on: Aug 17, 2022
 *      Author: marius
 */

#include "rpi_data_package.h"
#include <cmath>        // std::abs

RPIDataPackage::RPIDataPackage(uint8_t *buffer) {
	this->buffer = buffer;
	this->current_buffer = buffer;

}

RPIDataPackage::~RPIDataPackage() {
	// TODO Auto-generated destructor stub
}


uint32_t RPIDataPackage::nbr_of_bytes_to_send() {
	return std::abs(current_buffer - buffer);
}

