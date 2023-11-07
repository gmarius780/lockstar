/*
 * dac_new.hpp
 *
 *  Created on: 28.07.2022
 *      Author: Samuel
 */

#ifndef HAL_DACDEVICE_HPP_
#define HAL_DACDEVICE_HPP_

#include "main.h"
#include "dma.hpp"
#include "spi.hpp"

#include <algorithm>

typedef struct
{
    uint8_t dac_id;
    bool isBDMA;
    SPI_TypeDef *SPIx;
    BDMA_TypeDef *BDMAx;
    BDMA_Channel_TypeDef *BDMA_Channelx;
    LL_BDMA_InitTypeDef *BDMA_InitStruct;
    void (*bdma_clr_flag)(BDMA_TypeDef *BDMAx);
    DMA_TypeDef *DMAx;
    DMA_Stream_TypeDef *DMA_Streamx;
    LL_DMA_InitTypeDef *DMA_InitStruct;
    void (*dma_clr_flag)(DMA_TypeDef *DMAx);
    GPIO_TypeDef *sync_port;
    uint16_t sync_pin;
    GPIO_TypeDef *clear_port;
    uint16_t clear_pin;
    ADC_HandleTypeDef *STM_ADC;
    uint32_t SENH;
    uint32_t SENL;
} DAC_Device_TypeDef;

class DAC_Device
{
public:
    DAC_Device(uint8_t SPI, uint8_t dma_stream_out, uint8_t dma_channel_out, GPIO_TypeDef *sync_port, uint16_t sync_pin, GPIO_TypeDef *clear_port, uint16_t clear_pin);
    DAC_Device(uint8_t dac_id, GPIO_TypeDef *sync_port, uint16_t sync_pin, GPIO_TypeDef *clear_port, uint16_t clear_pin);
    DAC_Device(DAC_Device_TypeDef *DAC_conf);
    virtual void config_output();
    void config_output(ADC_HandleTypeDef *hadc, uint32_t ADC_SENL, uint32_t ADC_SENH);

    void write(float value);
    virtual void dma_transmission_callback();
    bool is_busy();
    void set_min_output(float m); // can only set the minimum higher than set with jumpers
    void set_max_output(float m); // can only set the maximum lower than set with jumpers
    float get_min_output();
    float get_max_output();

    float get_last_output();

protected:
    float inv_step_size;
    float step_size;
    float zero_voltage;
    float full_range;
    float max_output, min_output;
    float max_hardware_output, min_hardware_output; // as set with jumpers
    bool busy;
    bool invert;

    float last_output;

    GPIO_TypeDef *sync_port;
    uint16_t sync_pin;
    GPIO_TypeDef *clear_port;
    uint16_t clear_pin;

    DAC_Device_TypeDef *DAC_conf;
    void prepare_buffer();
    volatile uint8_t *dma_buffer;
    DMA *dma_output_handler;
    SPI *spi_handler;
    DMA_config_t dma_config;
    virtual void begin_dma_transfer();
};


class DAC1_Device: public DAC_Device
{
public:
    DAC1_Device(DAC_Device_TypeDef *DAC_conf);
    void dma_transmission_callback();
    // void write(float value);
    void config_output();


private:
    void begin_dma_transfer();
};
class DAC2_Device: public DAC_Device
{
public:
    DAC2_Device(DAC_Device_TypeDef *DAC_conf);
    void dma_transmission_callback();
    // void write(float value);
    void config_output();


private:
    void begin_dma_transfer();
};
// void arm_dma(DAC_Device_TypeDef *DAC_conf);
// void arm_bdma(DAC_Device_TypeDef *DAC_conf);
#endif /* HAL_DACDEVICE_HPP_ */
