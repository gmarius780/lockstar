/*
 * DoubleDitherLockModule.cpp
 *
 *  Implements two pid controllers that can be used for a PDH (Pound-Drever-Hall) type of Lock.
 *  Dithering to find the correct output-offset is implemented as well
 *
 *  Created on: Aug 18, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"

#include "../HAL/ADCDevice.hpp"

#include "../HAL/spi.hpp"
#include "../HAL/rpi.h"
#include "../HAL/leds.hpp"
#include "../HAL/BasicTimer.hpp"
#include "../HAL/DACDevice.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../Lib/pid.hpp"

#include "ScopeModule.h"

#ifdef DOUBLE_DITHER_LOCK_MODULE

enum ModuleState {
			BOTH_LOCKED, //both pid's running
			ONE_LOCKED,  //pid on input1/output1 running, dithering on output 2
			TWO_LOCKED,  //pid on input2/output2 running, dithering on output 1
			NONE_LOCKED  //dithering on both outputs
};

struct DitheringParams {
	float amp;
	float offset;
};

class DoubleDitherLockModule: public ScopeModule {
public:
	//number of increase/decrease steps in one dither period (triangular) (hardcoded for now)
	static const uint32_t NBR_DITHER_STEPS = 10000;
	//static const uint32_t DITHER_INCREASE_INTERVAL_100NS = 200; //increase the dithering after 20 us (200*100ns) --> this yields a dither-period of: NBR_DITHER_STEPS*DITHER_INCREASE_INTERVAL_100PS = 200ms,

	//loop timer oscillates with 1/100ns, the counter counts to 65535 --> the timer can capture approx 6.5ms --> enough to capture the dithering at 1.25ms steps
	static const uint16_t LOOP_TIMER_PSC = 9;
	static const uint16_t LOOP_TIMER_COUNTER_MAX = 0xFFFF;
	static const uint32_t LOOP_TIMER_FRQ = 90000000;

	DoubleDitherLockModule() : ScopeModule() {
		initialize_rpi();
		turn_LED6_off();
		turn_LED5_on();

		pid_one = new PID(0, 0, 0, 0, 0);
		pid_one->disable_intensity_mode();
		pid_two = new PID(0, 0, 0, 0, 0);
		pid_two->disable_intensity_mode();

		state = NONE_LOCKED;
		current_work_function = &DoubleDitherLockModule::work_none_locked;
		dither_one.amp = 0;
		dither_one.offset = 0;
		dither_two.amp = 0;
		dither_two.offset = 0;
		dither_step_increase_interval_100_ns = 200;

		setpoint_one = setpoint_two = 0;

		/*** TIMER FOR MAINLOOP TO EXTRACT DT ***/
		loop_timer = new BasicTimer(3, LOOP_TIMER_COUNTER_MAX, LOOP_TIMER_PSC);
		loop_timer->enable();
	}

	void run() {
		initialize_adc_dac(ADC_BIPOLAR_10V, ADC_BIPOLAR_10V);
		this->dac_1->write(0);
		this->dac_2->write(0);

		/*** work loop ***/
		while(true){
			(this->*current_work_function)(); //execute work function corresponding to current state
		}
	}

	/*** START: WORK METHODS - Switched according to current state of statemachine ***/

	/**
	 * Performs two PID loops when the state == BOTH_LOCKED
	 */
	void work_both_locked() {
		float dt = 0;
		uint16_t t = 0;
		loop_timer->reset_counter();

		//tell the compiler, the setpoint doesn't change during the loop
		float current_setpoint_one = this->setpoint_one;
		float current_setpoint_two = this->setpoint_two;

		//performs 10000 PID cycles to avoid jumping in and out of this function unnecessarily often as this will be the function which is run the most often
		//this loop should take 10000 * 10 us = 100ms
		this->scope->set_adc_active_mode(false); // no need to call adc->start_conversion() for the scope
		while (not this->scope->enable())  //reset the scope such that it records a full dither period
			HAL_Delay(10);
		for (int i = 0; i < 10000; i++) {
			// Measuring elapsed time per work loop
			t = loop_timer->get_counter() - t;
			dt = 1.0*t/LOOP_TIMER_FRQ*LOOP_TIMER_PSC;
			t = loop_timer->get_counter();
			this->adc->start_conversion();

			//could be optimized by making sure that the pid parameters don't change during the execution of the loop
			this->dac_1->write(this->pid_one->calculate_output(current_setpoint_one, adc->channel1->get_result(), dt));
			this->dac_2->write(this->pid_two->calculate_output(current_setpoint_two, adc->channel2->get_result(), dt));
		}
	}

	void work_one_locked() {
		//tell the compiler, the setpoint doesn't change during the loop
		float current_setpoint_one = this->setpoint_one;

		//step-size to achieve a full triangular period in <NBR_DITHER_STEPS> steps
		float dither_two_step = 2* this->dither_two.amp / NBR_DITHER_STEPS;
		float current_dither_two = this->dither_two.offset;

		float dt = 0;
		uint32_t t_since_last_dither_step = 0;
		uint16_t t = 0;

		loop_timer->reset_counter();
		uint32_t i = 0;
		this->scope->set_adc_active_mode(false); // no need to call adc->start_conversion() for the scope
		while (not this->scope->enable())  //reset the scope such that it records a full dither period
			HAL_Delay(10);
		while (i < NBR_DITHER_STEPS) {
			// Measuring elapsed time per work loop
			t = loop_timer->get_counter() - t;

			t_since_last_dither_step += t;

			//PERFORM DITHER STEP IF NECCESSARY
			if (t_since_last_dither_step >= dither_step_increase_interval_100_ns) {
				if (i < int(NBR_DITHER_STEPS/2)) {
					current_dither_two += dither_two_step;
				} else {
					current_dither_two -= dither_two_step;
				}

				this->dac_2->write(current_dither_two);
				t_since_last_dither_step = 0;
				i++;
			}

			//DO PID
			dt = 1.0*t/LOOP_TIMER_FRQ*LOOP_TIMER_PSC;
			t = loop_timer->get_counter();
			this->adc->start_conversion();
			//could be optimized by making sure that the pid parameters don't change during the execution of the loop
			this->dac_1->write(this->pid_one->calculate_output(current_setpoint_one, adc->channel1->get_result(), dt));
		}
	}

	void work_two_locked() {
		//tell the compiler, the setpoint doesn't change during the loop
		float current_setpoint_two = this->setpoint_two;

		//step-size to achieve a full triangular period in <NBR_DITHER_STEPS> steps
		float dither_one_step = 2* this->dither_one.amp / NBR_DITHER_STEPS;
		float current_dither_one = this->dither_one.offset;

		float dt = 0;
		uint32_t t_since_last_dither_step = 0;
		uint16_t t = 0;

		loop_timer->reset_counter();
		uint32_t i = 0;
		this->scope->set_adc_active_mode(false); // no need to call adc->start_conversion() for the scope
		while (not this->scope->enable())  //reset the scope such that it records a full dither period
			HAL_Delay(10);
		while (i < NBR_DITHER_STEPS) {
			// Measuring elapsed time per work loop
			t = loop_timer->get_counter() - t;

			t_since_last_dither_step += t;

			//PERFORM DITHER STEP IF NECCESSARY
			if (t_since_last_dither_step >= dither_step_increase_interval_100_ns) {
				if (i < int(NBR_DITHER_STEPS/2)) {
					current_dither_one += dither_one_step;
				} else {
					current_dither_one -= dither_one_step;
				}

				this->dac_1->write(current_dither_one);
				t_since_last_dither_step = 0;
				i++;
			}

			//DO PID
			dt = 1.0*t/LOOP_TIMER_FRQ*LOOP_TIMER_PSC;
			t = loop_timer->get_counter();
			this->adc->start_conversion();
			//could be optimized by making sure that the pid parameters don't change during the execution of the loop
			this->dac_2->write(this->pid_two->calculate_output(current_setpoint_two, adc->channel2->get_result(), dt));
		}
	}

	void work_none_locked() {
		//step-size to achieve a full triangular period in <NBR_DITHER_STEPS> steps
		float dither_one_step = 2 * this->dither_one.amp / NBR_DITHER_STEPS;
		float current_dither_one = this->dither_one.offset;

		float dither_two_step = 2 * this->dither_two.amp / NBR_DITHER_STEPS;
		float current_dither_two = this->dither_two.offset;

		uint32_t t_since_last_dither_step = 0;
		uint16_t t = 0;

		loop_timer->reset_counter();
		uint32_t i = 0;
		this->scope->set_adc_active_mode(true); //the scope has to call adc->start_conversion() since no pid is running
		while (not this->scope->enable())  //reset the scope such that it records a full dither period
			HAL_Delay(10);
		while (i < NBR_DITHER_STEPS) {
			// Measuring elapsed time per work loop
			t_since_last_dither_step += (loop_timer->get_counter() - t);
			t = loop_timer->get_counter();
			//PERFORM DITHER STEP IF NECCESSARY
			if (t_since_last_dither_step >= dither_step_increase_interval_100_ns) {
				if (i < int(NBR_DITHER_STEPS/2)) {
					current_dither_one += dither_one_step;
					current_dither_two += dither_two_step;
				} else {
					current_dither_one -= dither_one_step;
					current_dither_two -= dither_two_step;
				}

				this->dac_1->write(current_dither_one);
				this->dac_2->write(current_dither_two);
				t_since_last_dither_step = 0;
				i++;
			}
		}
	}


	/*** END: WORK METHODS ***/

	void handle_rpi_input() {
		if (ScopeModule::handle_rpi_base_methods() == false) { //if base class doesn't know the called method
			/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
			RPIDataPackage* read_package = rpi->get_read_package();

			// switch between method_identifier
			switch (read_package->pop_from_buffer<uint32_t>()) {
			case METHOD_SET_PID_ONE:
				set_pid_one(read_package);
				break;
			case METHOD_SET_PID_TWO:
				set_pid_two(read_package);
				break;
			case METHOD_LOCK_ONE:
				lock_one(read_package);
				break;
			case METHOD_LOCK_TWO:
				lock_two(read_package);
				break;
			case METHOD_UNLOCK_ONE:
				unlock_one(read_package);
				break;
			case METHOD_UNLOCK_TWO:
				unlock_two(read_package);
				break;
			case METHOD_SET_DITHER_ONE:
				set_dither_one(read_package);
				break;
			case METHOD_SET_DITHER_TWO:
				set_dither_two(read_package);
				break;
			case METHOD_SET_SETPOINT_ONE:
				set_setpoint_one(read_package);
				break;
			case METHOD_SET_SETPOINT_TWO:
				set_setpoint_two(read_package);
				break;
			case METHOD_SET_DITHER_FRQ:
				set_dither_frq(read_package);
				break;
			default:
				/*** send NACK because the method_identifier is not valid ***/
				RPIDataPackage* write_package = rpi->get_write_package();
				write_package->push_nack();
				rpi->send_package(write_package);
				break;
			}
		}
	}

	/*** START: METHODS ACCESSIBLE FROM THE RPI ***/
	static const uint32_t METHOD_SET_PID_ONE = 11;
	void set_pid_one(RPIDataPackage* read_package) {
		/***Read arguments***/
		float p = read_package->pop_from_buffer<float>();
		float i = read_package->pop_from_buffer<float>();
		float d = read_package->pop_from_buffer<float>();

		this->pid_one->set_pid(p, i, d, 0, this->dither_one.offset);

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_PID_TWO = 12;
	void set_pid_two(RPIDataPackage* read_package) {
		/***Read arguments***/
		float p = read_package->pop_from_buffer<float>();
		float i = read_package->pop_from_buffer<float>();
		float d = read_package->pop_from_buffer<float>();

		this->pid_two->set_pid(p, i, d, 0, this->dither_two.offset);

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_LOCK_ONE = 13;
	void lock_one(RPIDataPackage* read_package) {
		this->pid_one->set_output_offset(this->dither_one.offset);
		//state transition
		switch(state) {
		case NONE_LOCKED:
			state = ONE_LOCKED;
			current_work_function = &DoubleDitherLockModule::work_one_locked;
			break;
		case ONE_LOCKED:
			break;
		case TWO_LOCKED:
			state = BOTH_LOCKED;
			turn_LED6_on();
			current_work_function = &DoubleDitherLockModule::work_both_locked;
			break;
		case BOTH_LOCKED:
			break;
		}

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_LOCK_TWO = 14;
	void lock_two(RPIDataPackage* read_package) {
		this->pid_two->set_output_offset(this->dither_two.offset);
		//state transition
		switch(state) {
		case NONE_LOCKED:
			state = TWO_LOCKED;
			current_work_function = &DoubleDitherLockModule::work_two_locked;
			break;
		case ONE_LOCKED:
			state = BOTH_LOCKED;
			turn_LED6_on();
			current_work_function = &DoubleDitherLockModule::work_both_locked;
			break;
		case TWO_LOCKED:
			break;
		case BOTH_LOCKED:
			break;
		}

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_UNLOCK_ONE = 15;
	void unlock_one(RPIDataPackage* read_package) {
		//state transition
		switch(state) {
		case NONE_LOCKED:
			break;
		case ONE_LOCKED:
			state = NONE_LOCKED;
			current_work_function = &DoubleDitherLockModule::work_none_locked;
			break;
		case TWO_LOCKED:
			break;
		case BOTH_LOCKED:
			state = TWO_LOCKED;
			current_work_function = &DoubleDitherLockModule::work_two_locked;
			break;
		}
		turn_LED6_off();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_UNLOCK_TWO = 16;
	void unlock_two(RPIDataPackage* read_package) {
		//state transition
		switch(state) {
		case NONE_LOCKED:
			break;
		case ONE_LOCKED:
			break;
		case TWO_LOCKED:
			state = NONE_LOCKED;
			current_work_function = &DoubleDitherLockModule::work_none_locked;
			break;
		case BOTH_LOCKED:
			state = ONE_LOCKED;
			current_work_function = &DoubleDitherLockModule::work_one_locked;
			break;
		}
		turn_LED6_off();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_DITHER_ONE = 17;
	void set_dither_one(RPIDataPackage* read_package) {
		/***Read arguments***/
		float amp = read_package->pop_from_buffer<float>();
		float offset = read_package->pop_from_buffer<float>();

		this->dither_one.amp = amp;
		this->dither_one.offset = offset;

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_DITHER_TWO = 18;
	void set_dither_two(RPIDataPackage* read_package) {
		/***Read arguments***/
		float amp = read_package->pop_from_buffer<float>();
		float offset = read_package->pop_from_buffer<float>();

		this->dither_two.amp = amp;
		this->dither_two.offset = offset;

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_SETPOINT_ONE = 19;
	void set_setpoint_one(RPIDataPackage* read_package) {
		/***Read arguments***/
		float setpoint = read_package->pop_from_buffer<float>();

		this->setpoint_one = setpoint;

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_SETPOINT_TWO = 20;
	void set_setpoint_two(RPIDataPackage* read_package) {
		/***Read arguments***/
		float setpoint = read_package->pop_from_buffer<float>();

		this->setpoint_two = setpoint;

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_DITHER_FRQ = 21;
	void set_dither_frq(RPIDataPackage* read_package) {
		/***Read arguments***/
		float dither_frq_Hz = read_package->pop_from_buffer<float>();
		if (dither_frq_Hz <= 10) {
			this->dither_step_increase_interval_100_ns = uint32_t(1/dither_frq_Hz / NBR_DITHER_STEPS / 0.0000001);
			/*** send ACK ***/
			RPIDataPackage* write_package = rpi->get_write_package();
			write_package->push_ack();
			rpi->send_package(write_package);
		} else {
			/*** send NACK ***/
			RPIDataPackage* write_package = rpi->get_write_package();
			write_package->push_nack();
			rpi->send_package(write_package);
		}

	}

	/*** END: METHODS ACCESSIBLE FROM THE RPI ***/

	void rpi_dma_in_interrupt() {

		if(rpi->dma_in_interrupt())
		{ /*got new package from rpi*/
			handle_rpi_input();
		} else
		{ /* error */

		}
	}

public:
	PID *pid_one, *pid_two;
	ModuleState state;
	DitheringParams dither_one, dither_two;
	BasicTimer* loop_timer;
	float setpoint_one, setpoint_two;
	uint32_t dither_step_increase_interval_100_ns; //after dither_step_increase_interval_100_ns*100ns the dither signal will be increased by one step

	void(DoubleDitherLockModule::*current_work_function)();
};


DoubleDitherLockModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */



// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi
__attribute__((section("sram_func")))
__weak void DMA2_Stream4_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream2_IRQHandler(void)
{
	// SPI 1 rx
	module->adc->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream3_IRQHandler(void)
{
	// SPI 1 tx - SPI 5 rx
	// no action required
}
__attribute__((section("sram_func")))
void DMA2_Stream0_IRQHandler(void)
{
	module->rpi_dma_in_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	module->rpi->dma_out_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream6_IRQHandler(void)
{
	// SPI 6 Rx
	// no action required
}

__attribute__((section("sram_func")))
void SPI4_IRQHandler(void) {
	module->rpi->spi_interrupt();
}

__attribute__((section("sram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM4) {
		module->rpi->comm_reset_timer_interrupt();
	}
	else if(htim->Instance == TIM7) {
		module->scope_timer_interrupt();
	}
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{
	/* To speed up the access to functions, that are often called, we store them in the RAM instead of the FLASH memory.
	 * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
	 * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
	 * */
	extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
	volatile unsigned *src = &__sram_func_loadaddr;
	volatile unsigned *dest = &__sram_func_start;
	while (dest < &__sram_func_end) {
	  *dest = *src;
	  src++;
	  dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new DoubleDitherLockModule();

	module->run();

}

#endif
