/*
 * leds.cpp
 *
 *  Created on: 25 Feb 2020
 *      Author: qo
 */

#include "leds.hpp"

#include <main.h>

uint16_t get_led_pin(uint8_t led) {
  uint16_t pin;
  switch (led) {
  case 1:
    pin = LED1_Pin;
    break;
  case 2:
    pin = LED2_Pin;
    break;
  case 3:
    pin = LED3_Pin;
    break;
  case 4:
    pin = LED4_Pin;
    break;
  case 5:
    pin = LED5_Pin;
    break;
  case 6:
    pin = LED6_Pin;
    break;
  default:
    pin = 0;
  }
  return pin;
}

GPIO_TypeDef *get_led_port(uint8_t led) {
  GPIO_TypeDef *port;
  switch (led) {
  case 1:
    port = LED1_GPIO_Port;
    break;
  case 2:
    port = LED2_GPIO_Port;
    break;
  case 3:
    port = LED3_GPIO_Port;
    break;
  case 4:
    port = LED4_GPIO_Port;
    break;
  case 5:
    port = LED5_GPIO_Port;
    break;
  case 6:
    port = LED6_GPIO_Port;
    break;
  default:
    port = 0;
    break;
  }
  return port;
}

void turn_led_on(uint8_t led) {
  uint16_t pin = get_led_pin(led);
  GPIO_TypeDef *port = get_led_port(led);

  port->BSRR = pin;
}

void turn_led_off(uint8_t led) {
  uint16_t pin = get_led_pin(led);
  GPIO_TypeDef *port = get_led_port(led);

  port->BSRR = pin << 16U;
}

void turn_LED1_on() { LED1_GPIO_Port->BSRR = LED1_Pin; }

void turn_LED1_off() { LED1_GPIO_Port->BSRR = LED1_Pin << 16U; }

void turn_LED2_on() { LED2_GPIO_Port->BSRR = LED2_Pin; }

void turn_LED2_off() { LED2_GPIO_Port->BSRR = LED2_Pin << 16U; }

void turn_LED3_on() { LED3_GPIO_Port->BSRR = LED3_Pin; }

void turn_LED3_off() { LED3_GPIO_Port->BSRR = LED3_Pin << 16U; }

void turn_LED4_on() { LED4_GPIO_Port->BSRR = LED4_Pin; }

void turn_LED4_off() { LED4_GPIO_Port->BSRR = LED4_Pin << 16U; }

void turn_LED5_on() { LED5_GPIO_Port->BSRR = LED5_Pin; }

void turn_LED5_off() { LED5_GPIO_Port->BSRR = LED5_Pin << 16U; }

void turn_LED6_on() { LED6_GPIO_Port->BSRR = LED6_Pin; }

void turn_LED6_off() { LED6_GPIO_Port->BSRR = LED6_Pin << 16U; }
