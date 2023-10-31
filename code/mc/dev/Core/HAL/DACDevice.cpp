/*
 * dac_new.cpp
 *
 *  Created on: 28.07.2022
 *      Author: sjele
 */

#include "DACDevice.hpp"
#include "../Modules/dac_config.h"

__attribute__((section(".BDMABlock"))) uint8_t dma_buffer[3] = {0};

DAC_Device::DAC_Device(uint8_t spi_lane, uint8_t dma_stream_out, uint8_t dma_channel_out, GPIO_TypeDef *sync_port, uint16_t sync_pin, GPIO_TypeDef *clear_port, uint16_t clear_pin)
{
#ifndef IS_BDMA
    inv_step_size = 0;
    step_size = 0;
    zero_voltage = 0;
    full_range = 0;

    last_output = 0;

    max_output = 0;
    min_output = 0;
    busy = false;
    invert = false;
    dma_buffer = new uint8_t[3]();

    this->sync_port = sync_port;
    this->sync_pin = sync_pin;
    this->clear_port = clear_port;
    this->clear_pin = clear_pin;

    LL_DMA_InitTypeDef DMA_TX_InitStruct = {0};

    spi_handler = new SPI(DAC2_SPI);

    DMA_TX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi_handler->getTXDRAddress();
    DMA_TX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    DMA_TX_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_TX_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_TX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_TX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_TX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_TX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_TX_InitStruct.NbData = 3;
    DMA_TX_InitStruct.PeriphRequest = DMAMUX_REQ_DAC_SPI_TX;
    DMA_TX_InitStruct.Priority = LL_DMA_PRIORITY_MEDIUM;
    DMA_TX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
    DMA_TX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_TX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_TX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;

    dma_output_handler = new DMA(DAC2_DMA, DAC2_DMA_STREAM, &DMA_TX_InitStruct);
    dma_output_handler->enable_tc_irq();

    // Disable Clear-bit from start
    HAL_GPIO_WritePin(clear_port, clear_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(sync_port, sync_pin, GPIO_PIN_SET);
#endif
}

DAC_Device::DAC_Device(uint8_t dac_id, GPIO_TypeDef *sync_port, uint16_t sync_pin, GPIO_TypeDef *clear_port, uint16_t clear_pin)
{
#ifdef IS_BDMA
    inv_step_size = 0;
    step_size = 0;
    zero_voltage = 0;
    full_range = 0;

    last_output = 0;

    max_output = 0;
    min_output = 0;
    busy = false;
    invert = false;
    begin_dma_transfer = arm_bdma;

    this->sync_port = sync_port;
    this->sync_pin = sync_pin;
    this->clear_port = clear_port;
    this->clear_pin = clear_pin;

    spi_handler = new SPI(DAC2_SPI);

    // DAC1_conf.BDMA_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi_handler->getTXDRAddress();
    // DAC1_conf.BDMA_InitStruct.MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    // DAC1_conf.BDMA_InitStruct.NbData = 3;

    dma_output_handler = new DMA(DAC2_DMA, DAC2_DMA_STREAM, DAC1_conf.BDMA_InitStruct);
    LL_BDMA_EnableIT_TC(DAC2_DMA, DAC2_DMA_STREAM);
    LL_SPI_SetFIFOThreshold(DAC2_SPI, LL_SPI_FIFO_TH_03DATA);

    // Disable Clear-bit from start
    HAL_GPIO_WritePin(clear_port, clear_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(sync_port, sync_pin, GPIO_PIN_SET);
#endif
}

DAC_Device::DAC_Device(DAC_Device_TypeDef *DAC_conf)
{
    inv_step_size = 0;
    step_size = 0;
    zero_voltage = 0;
    full_range = 0;

    last_output = 0;

    max_output = 0;
    min_output = 0;
    busy = false;
    invert = false;
    begin_dma_transfer = arm_bdma;

    this->DAC_conf = DAC_conf;

    spi_handler = new SPI(DAC2_SPI);

    DAC_conf->BDMA_InitStruct->PeriphOrM2MSrcAddress = (uint32_t) &(DAC_conf->SPIx->TXDR);
    DAC_conf->BDMA_InitStruct->MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    DAC_conf->BDMA_InitStruct->NbData = 3;

    dma_output_handler = new DMA(DAC2_DMA, DAC2_DMA_STREAM, DAC_conf->BDMA_InitStruct);
    LL_BDMA_EnableIT_TC(DAC2_DMA, DAC2_DMA_STREAM);
    LL_SPI_SetFIFOThreshold(DAC2_SPI, LL_SPI_FIFO_TH_03DATA);

    // Disable Clear-bit from start
    HAL_GPIO_WritePin(DAC_conf->clear_port, DAC_conf->clear_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DAC_conf->sync_port, DAC_conf->sync_pin, GPIO_PIN_SET);
}

//__attribute__((section("sram_func")))
void DAC_Device::write(float output)
{
    if (busy)
        return;

    busy = true;

    output = std::min(max_output, std::max(output, min_output));
    last_output = output;

    int32_t int_output = (int32_t)((output - zero_voltage) * inv_step_size);

    // Bring SYNC line low to prepare DAC
    DAC_conf->sync_port->BSRR = (uint32_t)sync_pin << 16U;

    if (invert)
        int_output = -int_output;

    // Doesn't check that value is 20bit, may get unexpected results
    dma_buffer[0] = 0b00010000 + ((int_output >> 14) & 0x0f);
    dma_buffer[1] = (int_output >> 6) & 0xff;
    dma_buffer[2] = int_output & 0xff;

    begin_dma_transfer();
}

//__attribute__((section("sram_func")))
void DAC_Device::dma_transmission_callback()
{

    TX_DMA_ClearFlag(DAC2_DMA);
#ifdef IS_BDMA
    LL_BDMA_DisableChannel(DAC2_DMA, DAC2_DMA_STREAM);
    ATOMIC_MODIFY_REG(DAC2_TX_DMA_STREAM->CNDTR, BDMA_CNDTR_NDT, 3);
#else
    ATOMIC_MODIFY_REG(DAC2_TX_DMA_STREAM->NDTR, DMA_SxNDT, 3);
#endif
    while (!LL_SPI_IsActiveFlag_TXC(DAC2_SPI))
        ;
    ATOMIC_CLEAR_BIT(DAC2_SPI->CR1, SPI_CR1_SPE);
    /*
     * bring SYNC line up to finish DA conversion
     * (The DA conversion is completed automatically with the 24th transmitted bit.
     * The SYNC line has to go high at least 20ish ns before the next data package, so it
     * could also be done at a later point, if more convenient / faster.)
     */
    DAC_conf->sync_port->BSRR = (uint32_t)DAC_conf->sync_pin;

    busy = false;
}

void DAC_Device::config_output(ADC_HandleTypeDef *hadc, uint32_t ADC_SENL, uint32_t ADC_SENH)
{
    /*
     * The STM32 on-chip ADC (not the ADC_Device) is used to detect jumper configuration.
     * Based on this the output range of the DAC_Device is set
     */

    // read ADC value of lower voltage
    ADC_ChannelConfTypeDef adc_config = {0};
    adc_config.Channel = DAC2_SENL;
    adc_config.Rank = 1;
    adc_config.SamplingTime = ADC3_SAMPLETIME_2CYCLES_5;

    HAL_ADC_ConfigChannel(hadc, &adc_config);
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 1);

    uint32_t adc_result = HAL_ADC_GetValue(hadc);

    float low = 3.3 * adc_result / 4096.0f;
    if (low >= 2.6)
    {
        min_hardware_output = 0.0f;
        min_output = 0.0f;
    }
    else if (low < 2.6 && low >= 1.0)
    {
        min_hardware_output = -5.0f;
        min_output = -5.0f;
    }
    else if (low < 1.0)
    {
        min_hardware_output = -10.0f;
        min_output = -10.0f;
    }
    // read ADC value of upper voltage
    adc_config.Channel = DAC2_SENH;
    HAL_ADC_ConfigChannel(hadc, &adc_config);
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 1);

    adc_result = HAL_ADC_GetValue(hadc);

    float high = 3.3 * adc_result / 4096.0f;
    if (high >= 2.4)
    {
        max_hardware_output = 10.0f;
        max_output = 10.0f;
    }
    else if (high < 2.4)
    {
        max_hardware_output = 5.0f;
        max_output = 5.0f;
    }
    max_output = 10.0f;
    min_output = -10.0f;
    max_hardware_output = 10.0f;
    min_hardware_output = -10.0f;
    full_range = max_output - min_output;
    zero_voltage = (max_output + min_output) / 2.0f;
    step_size = full_range / 0x3ffff; // full_range / (2^20-1)
    inv_step_size = 1 / step_size;
    invert = false;

    send_output_range();
}

__attribute__((optimize(0))) void DAC_Device::send_output_range()
{
    busy = true;

    // bring SYNC line low to prepare DAC
    DAC_conf->sync_port->BSRR = (uint32_t)DAC_conf->sync_pin << 16U;

    // depending on the output range, the DAC applies a correction to improve linear behavior
    uint8_t comp = 0b0000;
    if (full_range < 10.0f)
        comp = 0b0000;
    else if (full_range < 12.0f)
        comp = 0b1001;
    else if (full_range < 16.0f)
        comp = 0b1010;
    else if (full_range < 19.0f)
        comp = 0b1011;
    else if (full_range >= 19.0f)
        comp = 0b1100;

    // construct control register, see datasheet
    bool RBUF = true;
    bool OPGND = false;
    bool DACTRI = false;
    bool NOT2C = false;
    bool SDODIS = false;
    volatile uint8_t control_reg = (RBUF << 1) + (OPGND << 2) + (DACTRI << 3) + (NOT2C << 4) + (SDODIS << 5);
    dma_buffer[0] = 0b00100000;
    dma_buffer[1] = comp >> 2;
    dma_buffer[2] = ((comp & 0b11) << 6) + control_reg;

    begin_dma_transfer();

    // Wait till configuration is sent
    while (busy)
        ;
}
//__attribute__((section("sram_func")))
void arm_dma()
{
    // LL_DMA_EnableStream(DAC2_DMA, DAC2_DMA_STREAM);
    // LL_SPI_EnableDMAReq_TX(DAC2_SPI);
    // while (!LL_DMA_IsEnabledStream(DAC2_DMA, DAC2_DMA_STREAM))
    // {
    // }
    // ATOMIC_SET_BIT(DAC2_SPI->CR1, SPI_CR1_SPE);
    // SET_BIT(DAC2_SPI->CR1, SPI_CR1_CSTART);
}

void arm_bdma()
{
    LL_BDMA_EnableChannel(DAC2_DMA, DAC2_DMA_STREAM);
    LL_SPI_EnableDMAReq_TX(DAC2_SPI);
    while (!LL_BDMA_IsEnabledChannel(DAC2_DMA, DAC2_DMA_STREAM))
    {
    }
    ATOMIC_SET_BIT(DAC2_SPI->CR1, SPI_CR1_SPE);
    SET_BIT(DAC2_SPI->CR1, SPI_CR1_CSTART);
}

bool DAC_Device::is_busy()
{
    return busy;
}

void DAC_Device::set_min_output(float m)
{
    this->min_output = std::max(this->min_hardware_output, m);
} // can only set the minimum higher than set with jumpers

void DAC_Device::set_max_output(float m)
{
    this->max_output = std::min(this->max_hardware_output, m);
} // can only set the maximum lower than set with jumpers

float DAC_Device::get_min_output()
{
    return this->min_output;
}

float DAC_Device::get_max_output()
{
    return this->max_output;
}

float DAC_Device::get_last_output()
{
    return this->last_output;
}
