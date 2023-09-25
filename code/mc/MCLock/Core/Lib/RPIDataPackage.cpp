/*
 * rpi_data_package.cpp
 *
 *  Created on: Aug 17, 2022
 *      Author: marius
 */

#include "RPIDataPackage.h"

#include <cmath>        // std::abs

RPIDataPackage::RPIDataPackage() {
	this->buffer = 0;
	this->current_buffer = 0;
}

RPIDataPackage::~RPIDataPackage() {

}


uint32_t RPIDataPackage::nbr_of_bytes_to_send() {
	return std::abs(current_buffer - buffer);
}

void RPIDataPackage::push_ack() {
	push_to_buffer<uint32_t>((uint32_t)221194);
}

void RPIDataPackage::push_nack() {
	push_to_buffer<uint32_t>((uint32_t)999999);
}

