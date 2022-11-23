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
		void new_linearization();
		void initialize_timer(RPIDataPackage*);
		void initialize_new_ramp(RPIDataPackage*);
		void set_ramp(RPIDataPackage*);
		void trigger_gain_measurement();
		void gain_measurement();
		void send_gain_measurement(RPIDataPackage*);
		void set_inverse_gain(RPIDataPackage*);
		enum METHOD_IDENTIFIER {
					NEW_LINEARIZATION = 11,
					INITIALIZE_TIMER = 12,
					INITIALIZE_NEW_RAMP = 13,
					SET_RAMP = 14,
					TRIGGER_GAIN_MEASUREMENT = 15,
					SEND_GAIN_MEASUREMENT = 16,
					SET_INVERSE_GAIN = 17
				};
		// END: rpi methods

		float linearize_output(float value);

		bool toggle;
		bool ready_to_work;
		bool received_new_ramp;
		bool timer_initialized;
		bool measurement_trigger;
		bool finished_gain_measurement;
		bool response_measurement_sent_to_rpi;
		bool received_inverse_gain;
		float ramp_output_range;

		uint32_t ramp_length;
		float* ramp_buffer;
		volatile uint32_t ramp_pointer;
		float* inverse_gain_buffer;
		BasicTimer* timer;
		uint32_t timer_psc;
		uint32_t timer_arr;

		float gain(float input);

	private:
		void reset_state_machine();


};
#endif
#endif /* MODULES_LINMODULE_H_ */

