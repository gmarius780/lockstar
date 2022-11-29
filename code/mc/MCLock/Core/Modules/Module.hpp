/*
 * Module.hpp
 *
 *  Created on: Feb 25, 2022
 *      Author: qo
 */

#ifndef MODULES_MODULE_HPP_
#define MODULES_MODULE_HPP_

#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"

#include <main.h>
#include "../HAL/spi.hpp"
#include "../HAL/leds.hpp"
#include "../HAL/adc_new.hpp"
#include "../HAL/dac_new.hpp"
#include "../HAL/rpi.h"

#include <cstring>

class Module {
public:
	Module();
	virtual ~Module();

	/*LINEARIZATION-METHODS START*/
	static const uint32_t METHOD_SET_LINEARIZATION_ONE = 80;
	void set_linearization_one(RPIDataPackage* read_package); //set calculated linearization parameters

	static const uint32_t START_LINEARIZATION_ONE = 81;
	static const uint32_t START_LINEARIZATION_TWO = 82;
	void start_linearization();
	/*LINEARIZATION-METHODS END*/

	void initialize_adc(uint8_t ch1_config, uint8_t ch2_config);
	void initialize_dac();
	void initialize_rpi();

	void set_ch_output_limit(RPIDataPackage* read_package, DAC_Device *dac);

	virtual bool handle_rpi_base_methods(); //handles calls send by the rpi corresponding to general methods

public:
	ADC_Device *adc;
	DAC_Device *dac_1, *dac_2;
	RPI *rpi;

	BasicTimer* lin_timer;

	bool is_linearizing; //true if linearization process is ongoing
	bool linearize_one; //to decide whether to linearize channel one or two
};

#endif /* MODULES_MODULE_HPP_ */
