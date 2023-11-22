/*
 * Timer.cpp
 *
 *  Created on: 11.08.2022
 *      Author: Samuel
 */

#include "BasicTimer.hpp"

BasicTimer::BasicTimer(uint8_t timer_x, uint32_t auto_reload, uint32_t prescaler)
{
	switch (timer_x)
	{
	case 2:
		tim_regs = TIM2;
		APBxLENR_bit = RCC_APB1LENR_TIM2EN;
		break;
	case 3:
		tim_regs = TIM3;
		APBxLENR_bit = RCC_APB1LENR_TIM3EN;
		break;
	case 4:
		tim_regs = TIM4;
		APBxLENR_bit = RCC_APB1LENR_TIM4EN;
		break;
	case 5:
		tim_regs = TIM5;
		APBxLENR_bit = RCC_APB1LENR_TIM5EN;
		break;
	case 6:
		tim_regs = TIM6;
		APBxLENR_bit = RCC_APB1LENR_TIM6EN;
		break;
	case 7:
		tim_regs = TIM7;
		APBxLENR_bit = RCC_APB1LENR_TIM7EN;
		break;
	default:
		tim_regs = 0;
	}

	LL_TIM_InitTypeDef TIM_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(APBxLENR_bit);

	TIM_InitStruct.Prescaler = prescaler;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = auto_reload;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(tim_regs, &TIM_InitStruct);

	// ARPE: Auto-reload preload enable - enable chaning of sampling rate on the fly
	LL_TIM_EnableARRPreload(tim_regs);
	LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
	LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIM2);
	disable_interrupt();
}

void BasicTimer::set_auto_reload(uint32_t value) { tim_regs->ARR = value; }

void BasicTimer::set_prescaler(uint32_t value) { tim_regs->PSC = value; }

uint32_t BasicTimer::get_counter() { return (uint32_t)tim_regs->CNT; }

void BasicTimer::enable() { tim_regs->CR1 |= (1 << 0); }

void BasicTimer::disable() { tim_regs->CR1 &= ~(1 << 0); }

void BasicTimer::enable_interrupt()
{
	tim_regs->EGR |= TIM_EGR_UG; // manually trigger update event, otherwise interrupt will be triggered immediately after enable
	reset_interrupt();
	tim_regs->DIER |= (1 << 0);
}

void BasicTimer::disable_interrupt() { tim_regs->DIER &= ~(1 << 0); }

void BasicTimer::reset_counter() { tim_regs->CNT = 0; }

void BasicTimer::reset_interrupt()
{
	//	tim_regs->SR &= ~TIM_SR_UIF;
	//	tim_regs->SR &= ~TIM_SR_TIF;
	tim_regs->SR &= 0;
}
