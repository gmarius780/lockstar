/*
 * adc_new.hpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#ifndef HAL_ADCDEVICE_HPP_
#define HAL_ADCDEVICE_HPP_

#include "stm32f4xx_hal.h"

#include "dma.hpp"
#include "spi.hpp"

#define ADC_BIPOLAR_10V     (uint8_t)0b111
#define ADC_BIPOLAR_5V      (uint8_t)0b011
#define ADC_UNIPOLAR_10V    (uint8_t)0b101
#define ADC_UNIPOLAR_5V     (uint8_t)0b001
#define ADC_OFF             (uint8_t)0b000

#define DATAWIDTH           6

class ADC_Device;

class ADC_Device_Channel {
public:
    ADC_Device_Channel(ADC_Device* parent_device, uint16_t channel_id, uint8_t setup);
    float get_result() { return result; }
    uint8_t get_channel_code() { return channel_code; }

    friend class ADC_Device;

private:
    void update_result(int16_t result);
    // Samuel: Maybe it makes more sense to move the channel buffer to the modules using the ADC?
    float result;
    float step_size;
    bool two_comp;
    ADC_Device *parent_device;
    uint8_t channel_id, channel_code;
};

class ADC_Device {
public:
    ADC_Device(uint8_t spi_lane, 
                uint8_t dma_stream_in,
                uint8_t dma_channel_in,
                uint8_t dma_stream_out,
                uint8_t dma_channel_out,
                GPIO_TypeDef* cnv_port,
                uint16_t cnv_pin,
                uint8_t channel1_config,
                uint8_t channel2_config);

    ADC_Device_Channel *channel1, *channel2;
    void start_conversion();
    void dma_transmission_callback();
    bool is_busy() { return busy; };
    SPI *spi_handler;
private:
    void arm_dma();
    void disarm_dma();

    volatile uint8_t* dma_buffer;
    volatile uint8_t* adc_config_buffer;

    uint16_t cnv_pin;
    GPIO_TypeDef* cnv_port;

    bool single_channel_mode;
    bool busy;

    DMA *dma_input_handler, *dma_output_handler;
    //SPI *spi_handler;
    DMA_config_t dma_in_config, dma_out_config;
};



#endif /* HAL_ADCDEVICE_HPP_ */
