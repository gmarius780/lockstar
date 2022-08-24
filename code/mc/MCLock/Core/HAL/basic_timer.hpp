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
	BasicTimer(uint8_t timer_x, uint32_t auto_reload, uint32_t prescaler, bool interrupt);
	void update_frequency(float f);
	void set_auto_reload(uint32_t value);
	void set_prescaler(uint32_t value);
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
