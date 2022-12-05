/*
 * Module.cpp
 *
 *  Created on: Feb 25, 2022
 *      Author: qo
 */

#include "Module.hpp"

extern ADC_HandleTypeDef hadc3;

Module::Module() {

}

Module::~Module() {
	// TODO Auto-generated destructor stub
}

/*
 * Checks if the called method is implemented in this class. If not it returns false such that
 * the derived classes know that they need to check, if they know the method
 */
bool Module::handle_rpi_base_methods() {
	/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
	RPIDataPackage* read_package = rpi->get_read_package();
	switch (read_package->pop_from_buffer<uint32_t>()) {
	case METHOD_SET_CH_ONE_OUTPUT_LIMITS:
		set_ch_one_output_limits(read_package);
		break;
	case METHOD_SET_CH_TWO_OUTPUT_LIMITS:
		set_ch_two_output_limits(read_package);
		break;
	case METHOD_SET_LINEARIZATION_ONE:
		set_linearization_one(read_package);
		break;
	case METHOD_SET_LINEARIZATION_TWO:
		set_linearization_two(read_package);
		break;
	case METHOD_SET_LINEARIZATION_LENGTH_ONE:
		set_linearization_length_one(read_package);
		break;
	case METHOD_SET_LINEARIZATION_LENGTH_TWO:
		set_linearization_length_two(read_package);
		break;
	case METHOD_ENABLE_LINEARIZATION_ONE:
		enable_linearization_one(read_package);
		break;
	case METHOD_ENABLE_LINEARIZATION_TWO:
		enable_linearization_two(read_package);
		break;
	case METHOD_DISABLE_LINEARIZATION_ONE:
		disable_linearization_one(read_package);
		break;
	case METHOD_DISABLE_LINEARIZATION_TWO:
		disable_linearization_two(read_package);
		break;
	default:
		return false;
	}

	return true;
}

void Module::initialize_adc(uint8_t ch1_config, uint8_t ch2_config) {
	adc = new ADC_Device(	/* SPI number */ 				1,
							/* DMA Stream In */ 			2,
							/* DMA Channel In */ 			3,
							/* DMA Stream Out */ 			3,
							/* DMA Channel Out */ 			3,
							/* conversion pin port */ 		ADC_CNV_GPIO_Port,
							/* conversion pin number */		ADC_CNV_Pin,
							/* Channel 1 config */			ch1_config,
							/* Channel 2 config */			ch2_config);
}

void Module::initialize_dac() {
	dac_1 = new LinearizableDAC( /*SPI number*/              6,
							/*DMA Stream Out*/          5,
							/*DMA Channel Out*/         1,
							/* sync pin port*/          DAC_1_Sync_GPIO_Port,
							/* sync pin number*/        DAC_1_Sync_Pin,
							/* clear pin port*/         CLR6_GPIO_Port,
							/* clear pin number*/       CLR6_Pin);

	dac_2 = new LinearizableDAC( /* SPI number*/				5,
							/* DMA Stream Out*/			4,
							/* DMA Channel Out*/		2,
							/* sync pin port*/          DAC_2_Sync_GPIO_Port,
							/* sync pin number*/        DAC_2_Sync_Pin,
							/* clear pin port*/         CLR5_GPIO_Port,
							/* clear pin number*/       CLR5_Pin);

	dac_1->config_output(&hadc3, ADC_CHANNEL_14, ADC_CHANNEL_9);
	dac_2->config_output(&hadc3, ADC_CHANNEL_8, ADC_CHANNEL_15);
}

void Module::initialize_rpi() {
	rpi = new RPI();
}

/** RPI METHODS START***/

/**
 * expects two floats: the first corresponds to the minimum the second to the maximum of output one
 */
void Module::set_ch_one_output_limits(RPIDataPackage* read_package) {
	set_ch_output_limit(read_package, this->dac_1);
}

/**
 * expects two floats: the first corresponds to the minimum the second to the maximum of output two
 */
void Module::set_ch_two_output_limits(RPIDataPackage* read_package) {
	set_ch_output_limit(read_package, this->dac_2);
}

void Module::set_ch_output_limit(RPIDataPackage* read_package, LinearizableDAC *dac) {
	/***Read arguments***/
	dac->set_min_output(read_package->pop_from_buffer<float>());
	dac->set_max_output(read_package->pop_from_buffer<float>());

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}


/*LINEARIZATION-METHODS START*/

/**
 * Set the linearization as gotten from the linearization module
 *
 * Parameters:
 *
 * (float) min_output_voltage: starting value of the ramp that was used for the linearization
 * (float) max_output_voltage: end value of the ramp that was used for the linearization
 * (bool) append: wheter the comming values are to be appended to the linearization_buffer of the dac
 * (uint32_t) package_size: number of pivots to be added to the linearization_buffer
 * (float*) pivot points
 */
void Module::set_linearization_one(RPIDataPackage* read_package) {
	set_linearization(read_package, dac_1);
}

/**
 * Set the linearization as gotten from the linearization module
 *
 * Parameters:
 *
 * (float) min_output_voltage: starting value of the ramp that was used for the linearization
 * (float) max_output_voltage: end value of the ramp that was used for the linearization
 * (bool) append: wheter the comming values are to be appended to the linearization_buffer of the dac
 * (uint32_t) package_size: number of pivots to be added to the linearization_buffer
 * (float*) pivot points
 */
void Module::set_linearization_two(RPIDataPackage* read_package) {
	set_linearization(read_package, dac_2);
}

void Module::set_linearization(RPIDataPackage* read_package, LinearizableDAC *dac) {
	float min_output_voltage = read_package->pop_from_buffer<float>();
	float max_output_voltage = read_package->pop_from_buffer<float>();
	bool append = read_package->pop_from_buffer<bool>();
	uint32_t package_size = read_package->pop_from_buffer<uint32_t>();

	dac->set_min_output(min_output_voltage);
	dac->set_max_output(max_output_voltage);

	//push the received values onto the linearization buffer of the given dac
	bool success = true;
	for(uint32_t i=0; i<package_size; i++) {
		if (dac->push_to_linearization_buffer(read_package->pop_from_buffer<float>(), true ? (append and i==0):false) == false) {
			success = false;
			break;
		}
	}


	RPIDataPackage* ack = rpi->get_write_package();
	if (success) {
		ack->push_ack();
	} else {
		ack->push_nack();
	}
	rpi->send_package(ack);
}

/**
 * initializes the buffer, used to store the pivot points with the given length. This method should ONLY be called when
 * once or whenever you change the buffer length, as it instantiates a new buffer every time. if you do this too many times,
 * you will run out of RAM.
 *
 * expects:
 *  (uint32_t) buffer_length
 */
void Module::set_linearization_length_one(RPIDataPackage* read_package) {
	/***Read arguments***/
	uint32_t linearization_buffer_length = read_package->pop_from_buffer<uint32_t>();

	dac_1->set_linearization_length(linearization_buffer_length);

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}

/**
 * initializes the buffer, used to store the pivot points with the given length. This method should ONLY be called when
 * once or whenever you change the buffer length, as it instantiates a new buffer every time. if you do this too many times,
 * you will run out of RAM.
 *
 * expects:
 *  (uint32_t) buffer_length
 */
void Module::set_linearization_length_two(RPIDataPackage* read_package) {
	/***Read arguments***/
	uint32_t linearization_buffer_length = read_package->pop_from_buffer<uint32_t>();
	dac_2->set_linearization_length(linearization_buffer_length);

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}

void Module::enable_linearization_one(RPIDataPackage* read_package) {

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();

	if (dac_1->enable_linearization()) {
		write_package->push_ack();
	} else {
		write_package->push_nack();
	}
	rpi->send_package(write_package);
}

void Module::enable_linearization_two(RPIDataPackage* read_package) {

	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();

	if (dac_2->enable_linearization()) {
		write_package->push_ack();
	} else {
		write_package->push_nack();
	}
	rpi->send_package(write_package);
}

void Module::disable_linearization_one(RPIDataPackage* read_package) {
	dac_1->disable_linearization();
	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}

void Module::disable_linearization_two(RPIDataPackage* read_package) {
	dac_2->disable_linearization();
	/*** send ACK ***/
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}
/*LINEARIZATION-METHODS END*/

/** RPI METHODS END***/

