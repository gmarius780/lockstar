#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"

#include "Module.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../HAL/BasicTimer.hpp"
#include "../HAL/leds.hpp"

#include <math.h>

#ifndef MODULES_LINMODULE_H_
#define MODULES_LINMODULE_H_

#ifdef LINEARIZATION_MODULE

class LinearizationModule: public Module {

	public:
		LinearizationModule();
		void handle_rpi_input();
		void rpi_dma_in_interrupt();
		void measure_response();
		void run();

		// START: rpi methods
		static const uint32_t SET_RAMP_PARAMETERS = 11;
		void set_ramp_parameters(RPIDataPackage*);
		static const uint32_t SEND_GAIN_MEASUREMENT = 12;
		void send_gain_measurement(RPIDataPackage*);
		static const uint32_t SET_INVERTED_PIVOTS = 13;
		void set_inverted_pivots(RPIDataPackage*);
		// END: rpi methods

		float linearize_output(float value);
		void new_linearization();
		void perform_gain_measurement();

		bool ready_to_work;
		bool received_ramp_paramters;
		bool finished_gain_measurement;
		bool sent_gain_measurement;
		bool received_inverted_pivots;
		bool linearization_active;

		float output_range;
		float output_min;
		float output_max;
		float ramp_range;
		float pivot_spacing;

		bool test;

		float ramp_start;
		float ramp_end;
		float ramp_stepsize;
		uint32_t ramp_length;
		volatile uint32_t buffer_pointer;
		float* gain_measurement_buffer;
		float* inverted_pivots_buffer;

		BasicTimer* timer;
		uint32_t timer_psc;
		uint32_t timer_arr;

	private:
		void reset_state_machine();


};
#endif
#endif /* MODULES_LINMODULE_H_ */

