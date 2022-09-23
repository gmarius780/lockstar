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

	void initialize_adc(uint8_t ch1_config, uint8_t ch2_config);
	void initialize_dac();
	void initialize_rpi();

	void set_ch_output_limit(RPIDataPackage* read_package, DAC_Device *dac);

public:
	ADC_Device *adc;
	DAC_Device *dac_1, *dac_2;
	RPI *rpi;
};

#endif /* MODULES_MODULE_HPP_ */
