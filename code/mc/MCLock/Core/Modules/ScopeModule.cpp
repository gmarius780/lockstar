/*
 * ScopeModule.cpp
 *
 *  Created on: Dec 28, 2022
 *      Author: marius
 */

#include "ScopeModule.h"

ScopeModule::ScopeModule() {
}

ScopeModule::~ScopeModule() {
}

void ScopeModule::initialize_adc_dac(uint8_t ch1_config, uint8_t ch2_config) {
	Module::initialize_adc_dac(ch1_config, ch2_config);
	this->scope = new Scope(this->adc, this->dac_1, this->dac_2);
}

/**Scope methods start**/
void ScopeModule::setup_scope(RPIDataPackage* read_package) {
	/***Read arguments***/
	uint32_t sampling_prescaler = read_package->pop_from_buffer<uint32_t>();
	uint32_t sampling_counter_max = read_package->pop_from_buffer<uint32_t>();

	uint32_t nbr_samples_in_one = read_package->pop_from_buffer<uint32_t>();
	uint32_t nbr_samples_in_two = read_package->pop_from_buffer<uint32_t>();
	uint32_t nbr_samples_out_one = read_package->pop_from_buffer<uint32_t>();
	uint32_t nbr_samples_out_two = read_package->pop_from_buffer<uint32_t>();

	bool adc_active_mode = read_package->pop_from_buffer<bool>();
	bool double_buffer_mode = read_package->pop_from_buffer<bool>();

	bool success = scope->setup_scope(sampling_prescaler, sampling_counter_max, nbr_samples_in_one, nbr_samples_in_two,  \
			nbr_samples_out_one, nbr_samples_out_two, adc_active_mode, double_buffer_mode);

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	if (success)
		write_package->push_ack();
	else
		write_package->push_nack();
	rpi->send_package(write_package);
}

void ScopeModule::set_scope_sampling_rate(RPIDataPackage* read_package) {
	/***Read arguments***/
	uint32_t sampling_prescaler = read_package->pop_from_buffer<uint32_t>();
	uint32_t sampling_counter_max = read_package->pop_from_buffer<uint32_t>();

	bool success = scope->set_sampling_rate(sampling_prescaler, sampling_counter_max);

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	if (success)
		write_package->push_ack();
	else
		write_package->push_nack();
	rpi->send_package(write_package);
}

void ScopeModule::enable_scope(RPIDataPackage* read_package) {
	bool success = scope->enable();

	/*** send ACK/NACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	if (success)
		write_package->push_ack();
	else
		write_package->push_nack();
	rpi->send_package(write_package);
}

void ScopeModule::disable_scope(RPIDataPackage* read_package) {
	bool success = scope->disable();

	/*** send ACK/NACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	if (success)
		write_package->push_ack();
	else
		write_package->push_nack();
	rpi->send_package(write_package);
}

void ScopeModule::get_scope_data(RPIDataPackage* read_package) {
	/***Read arguments***/
	uint32_t buffer_offset = read_package->pop_from_buffer<uint32_t>();
	uint32_t package_size = read_package->pop_from_buffer<uint32_t>();

	RPIDataPackage* write_package = rpi->get_write_package();

	bool success = scope->push_buffers_to_rpi_data_package(write_package, buffer_offset, package_size);

	/*** send ACK/NACK ***/
	if (success == false) {
		RPIDataPackage* nack = rpi->get_write_package();
		nack->push_nack();
		rpi->send_package(nack);
	} else {
		rpi->send_package(write_package);
	}
}
/**Scope methods end**/

void ScopeModule::scope_timer_interrupt() {
	scope->timer_interrupt();
}


bool ScopeModule::handle_rpi_base_methods() {

	if (Module::handle_rpi_base_methods() == false) { //if base class doesn't know the called method
		/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
		RPIDataPackage* read_package = rpi->get_read_package();

		// switch between method_identifier
		switch (read_package->pop_from_buffer<uint32_t>()) {
		case METHOD_SETUP_SCOPE:
			setup_scope(read_package);
			break;
		case METHOD_ENABLE_SCOPE:
			enable_scope(read_package);
			break;
		case METHOD_DISABLE_SCOPE:
			disable_scope(read_package);
			break;
		case METHOD_GET_SCOPE_DATA:
			get_scope_data(read_package);
			break;
		case METHOD_SET_SCOPE_SAMPLING_RATE:
			set_scope_sampling_rate(read_package);
			break;
		default:
			return false;
		}

		return true;
	} else {
		return true;
	}
}
