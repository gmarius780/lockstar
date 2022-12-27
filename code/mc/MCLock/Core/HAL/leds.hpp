/*
 * leds.hpp
 *
 *  Created on: 25 Feb 2020
 *      Author: qo
 */

#ifndef HAL_LEDS_HPP_
#define HAL_LEDS_HPP_

#include "stm32f427xx.h"


void turn_led_on(uint8_t);
void turn_led_off(uint8_t);
uint16_t get_led_pin();
GPIO_TypeDef* get_led_port();

void turn_LED1_on();
void turn_LED1_off();
void turn_LED2_on();
void turn_LED2_off();
void turn_LED3_on();
void turn_LED3_off();
void turn_LED4_on();
void turn_LED4_off();
void turn_LED5_on();
void turn_LED5_off();
void turn_LED6_on();
void turn_LED6_off();


#endif /* HAL_LEDS_HPP_ */
