/*
 * leds.cpp
 *
 *  Created on: 25 Feb 2020
 *      Author: qo
 */

#include <main.h>
#include <leds.hpp>

void turn_LED1_on()
{
	LED1_GPIO_Port->BSRR = LED1_Pin;
}

void turn_LED1_off()
{
	LED1_GPIO_Port->BSRR = LED1_Pin << 16U;
}

void turn_LED2_on()
{
	LED2_GPIO_Port->BSRR = LED2_Pin;
}

void turn_LED2_off()
{
	LED2_GPIO_Port->BSRR = LED2_Pin << 16U;
}

void turn_LED3_on()
{
	LED3_GPIO_Port->BSRR = LED3_Pin;
}

void turn_LED3_off()
{
	LED3_GPIO_Port->BSRR = LED3_Pin << 16U;
}

void turn_LED4_on()
{
	LED4_GPIO_Port->BSRR = LED4_Pin;
}

void turn_LED4_off()
{
	LED4_GPIO_Port->BSRR = LED4_Pin << 16U;
}

void turn_LED5_on()
{
	LED5_GPIO_Port->BSRR = LED5_Pin;
}

void turn_LED5_off()
{
	LED5_GPIO_Port->BSRR = LED5_Pin << 16U;
}

void turn_LED6_on()
{
	LED6_GPIO_Port->BSRR = LED6_Pin;
}

void turn_LED6_off()
{
	LED6_GPIO_Port->BSRR = LED6_Pin << 16U;
}
