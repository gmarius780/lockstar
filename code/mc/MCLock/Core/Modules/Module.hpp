/*
 * Module.hpp
 *
 *  Created on: Feb 25, 2022
 *      Author: qo
 */

#ifndef MODULES_MODULE_HPP_
#define MODULES_MODULE_HPP_

#include "../Inc/main.h"
#include "../HAL/adc.hpp"
#include "../HAL/dac.hpp"
#include "../HAL/raspberrypi.hpp"
#include "../HAL/leds.hpp"
#include "../Inc/misc_func.hpp"

#include <cstring>

extern ADC_HandleTypeDef hadc3;

enum ModuleState {module_state_rpi_init_pending, module_state_running, module_state_failed};
enum RpiInputState {rpi_input_pending, rpi_input_none};

class Module {
public:
	Module();
	virtual ~Module();

	void run();
	virtual void init();
	void loop();

	void adc_interrupt();
	void rpi_interrupt();
	void trigger_interrupt();
	void timer_interrupt();

	virtual void handle_rpi_input();
	void rpi_init();
	virtual void work();

	DAC_Dev *dac_2, *dac_1;
	ADC_Dev *adc_dev;
	RaspberryPi *rpi;

	ModuleState module_state;
	RpiInputState rpi_input_state;

};

#endif /* MODULES_MODULE_HPP_ */
