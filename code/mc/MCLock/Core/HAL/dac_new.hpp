/*
 * dac_new.hpp
 *
 *  Created on: 28.07.2022
 *      Author: Samuel
 */

#ifndef HAL_DAC_NEW_HPP_
#define HAL_DAC_NEW_HPP_

#include "stm32f4xx_hal.h"
#include "dma_new.hpp"
#include "dma.hpp"
#include "spi.hpp"
#include <algorithm>

class DAC_Device {
public:
	DAC_Device(uint8_t SPI, uint8_t dma_stream_out, uint8_t dma_channel_out, GPIO_TypeDef* sync_port, uint16_t sync_pin, GPIO_TypeDef* clear_port, uint16_t clear_pin);
    void config_output(ADC_HandleTypeDef* hadc, uint32_t ADC_SENL, uint32_t ADC_SENH);

    void write(float value);
    void dma_transmission_callback();
    bool is_busy() { return busy; }
    void set_min_output(float m) {this->min_output = std::max(this->min_hardware_output, m);} // can only set the minimum higher than set with jumpers
    void set_max_output(float m) {this->max_output = std::min(this->max_hardware_output, m);} // can only set the maximum lower than set with jumpers
    float get_min_output() {return this->min_output;}
    float get_max_output() {return this->max_output;}

private:
    float inv_step_size;
    float step_size;
    float zero_voltage;
    float full_range;
    float max_output, min_output;
    float max_hardware_output, min_hardware_output; // as set with jumpers
    bool busy;
    bool invert;

    GPIO_TypeDef* sync_port;
	uint16_t sync_pin;
	GPIO_TypeDef* clear_port;
	uint16_t clear_pin;

    void send_output_range();
    void arm_dma();
    volatile uint8_t* dma_buffer;
    DMA* dma_output_handler;
    SPI_DMA_Handler* old_dma;
    SPI* spi_handler;
    DMA_config_t dma_config;
};

#endif /* HAL_DAC_NEW_HPP_ */
