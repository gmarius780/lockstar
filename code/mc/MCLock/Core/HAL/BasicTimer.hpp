/*
 * Timer.hpp
 *
 *  Created on: 11.08.2022
 *      Author: sjele
 */

#ifndef HAL_BASICTIMER_HPP_
#define HAL_BASICTIMER_HPP_

#include "stm32f427xx.h"

/*
 * Timers are used at the following places so far:
 *
 * Timer 2: Sampling timer for the AWG/AWGPID Module
 * Timer 3: For the PID modules to get dt
 * Timer 4: For the raspberry pi to reset 'is_communicating' in case of communication failure
 * */
class BasicTimer {
public:
	static const uint32_t INTERNAL_CLOCK_FREQUENCY = 90e6;

	BasicTimer(uint8_t timer_x, uint32_t auto_reload, uint32_t prescaler);
	void set_auto_reload(uint32_t value); //specifies after how many counts at base_frequency/prescaler the counter should be reset --> interrupt is triggered
	void set_prescaler(uint32_t value); //scales the counter frequency down from base_frequency = INTERNAL_CLOCK_FREQUENCY
	uint32_t get_counter();
	void enable();
	void disable();
	void enable_interrupt();
	void disable_interrupt();

	void reset_counter();
	void reset_interrupt();

private:
	TIM_TypeDef* tim_regs;
	uint32_t APBxENR_bit;

};

#endif /* HAL_BASICTIMER_HPP_ */
