/*
 * Timer.hpp
 *
 *  Created on: 11.08.2022
 *      Author: sjele
 */

#ifndef HAL_BASIC_TIMER_HPP_
#define HAL_BASIC_TIMER_HPP_

#include "stm32f427xx.h"

class BasicTimer {
public:
	static const uint32_t INTERNAL_CLOCK_FREQUENCY = 90e6;

	BasicTimer(uint8_t timer_x, uint32_t auto_reload, uint32_t prescaler, bool interrupt);
	void set_auto_reload(uint32_t value); //specifies after how many counts at base_frequency/prescaler the counter should be reset --> interrupt is triggered
	void set_prescaler(uint32_t value); //scales the counter frequency down from base_frequency = INTERNAL_CLOCK_FREQUENCY
	uint32_t get_counter();
	void enable();
	void disable();
	void enable_interrupt();
	void disable_interrupt();

private:
	TIM_TypeDef* tim_regs;
	uint32_t APBxENR_bit;
	uint32_t PSC;
	uint32_t ARR;

};

#endif /* HAL_BASIC_TIMER_HPP_ */
