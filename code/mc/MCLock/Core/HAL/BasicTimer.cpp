/*
 * Timer.cpp
 *
 *  Created on: 11.08.2022
 *      Author: Samuel
 */

#include "BasicTimer.hpp"

BasicTimer::BasicTimer(uint8_t timer_x, uint32_t auto_reload, uint32_t prescaler) {
	switch(timer_x) {
		case 2: tim_regs = TIM2; APBxENR_bit = RCC_APB1ENR_TIM2EN; break;
		case 3: tim_regs = TIM3; APBxENR_bit = RCC_APB1ENR_TIM3EN; break;
		case 4: tim_regs = TIM4; APBxENR_bit = RCC_APB1ENR_TIM4EN; break;
		case 5: tim_regs = TIM5; APBxENR_bit = RCC_APB1ENR_TIM5EN; break;
		case 7: tim_regs = TIM7; APBxENR_bit = RCC_APB1ENR_TIM7EN; break;
		default: tim_regs = 0;
	}

	RCC->APB1ENR |= (1<<APBxENR_bit);


	//ARPE: Auto-reload preload enable - enable chaning of sampling rate on the fly
	//tim_regs->CR1 |= (1<<7);
	tim_regs->CR1 |= TIM_CR1_ARPE;

	set_auto_reload(auto_reload);
	set_prescaler(prescaler);
	disable_interrupt();
	tim_regs->EGR |= TIM_EGR_UG; //manually trigger update event, otherwise interrupt will be triggered immediately after enable
}

void BasicTimer::set_auto_reload(uint32_t value) { tim_regs->ARR = value; }

void BasicTimer::set_prescaler(uint32_t value) { tim_regs->PSC = value; }

__attribute__((section("sram_func")))
uint32_t BasicTimer::get_counter() { return (uint32_t)tim_regs->CNT; }

void BasicTimer::enable() { tim_regs->CR1 |= (1<<0); }

void BasicTimer::disable() { tim_regs->CR1 &= ~(1<<0); }

void BasicTimer::enable_interrupt() {
	tim_regs->EGR |= TIM_EGR_UG; //manually trigger update event, otherwise interrupt will be triggered immediately after enable
	reset_interrupt();
	tim_regs->DIER |= (1<<0);
}

void BasicTimer::disable_interrupt() { tim_regs->DIER &= ~(1<<0); }

void BasicTimer::reset_counter() {tim_regs->CNT = 0;}

void BasicTimer::reset_interrupt() {
//	tim_regs->SR &= ~TIM_SR_UIF;
//	tim_regs->SR &= ~TIM_SR_TIF;
	tim_regs->SR &= 0;
}

