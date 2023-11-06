/*
 * dac_new.cpp
 *
 *  Created on: 28.07.2022
 *      Author: sjele
 */

#include "DACDevice.hpp"

__attribute__((section(".data"))) uint8_t dmaD1_buffer[3] = {0};
__attribute__((section(".BDMABlock"))) uint8_t dmaD3_buffer[3] = {0};

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

    this->DAC_conf = DAC_conf;
    spi_handler = new SPI(DAC_conf->SPIx);
    LL_SPI_SetFIFOThreshold(DAC_conf->SPIx, LL_SPI_FIFO_TH_03DATA);
    LL_SPI_SetTransferSize(DAC_conf->SPIx, 3);

    // Disable Clear-bit from start
    HAL_GPIO_WritePin(DAC_conf->clear_port, DAC_conf->clear_pin, GPIO_PIN_SET);
}

DAC1_Device::DAC1_Device(DAC_Device_TypeDef *DAC_conf) : DAC_Device(DAC_conf)
{
    dma_buffer = dmaD3_buffer;
    DAC_conf->BDMA_InitStruct->PeriphOrM2MSrcAddress = (uint32_t) & (DAC_conf->SPIx->TXDR);
    DAC_conf->BDMA_InitStruct->MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    DAC_conf->BDMA_InitStruct->NbData = 3;
    LL_BDMA_Init(DAC_conf->BDMAx, __LL_BDMA_GET_CHANNEL(DAC_conf->BDMA_Channelx), DAC_conf->BDMA_InitStruct);
    // EnableIT_TC(DAC_conf->BDMA_Channelx);
    LL_SPI_EnableIT_EOT(DAC_conf->SPIx);
}
DAC2_Device::DAC2_Device(DAC_Device_TypeDef *DAC_conf) : DAC_Device(DAC_conf)
{
    dma_buffer = dmaD1_buffer;
    DAC_conf->DMA_InitStruct->PeriphOrM2MSrcAddress = (uint32_t) & (DAC_conf->SPIx->TXDR);
    DAC_conf->DMA_InitStruct->MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    DAC_conf->DMA_InitStruct->NbData = 3;
    LL_DMA_Init(DAC_conf->DMAx, __LL_DMA_GET_STREAM(DAC_conf->DMA_Streamx), DAC_conf->DMA_InitStruct);
    // EnableIT_TC(DAC_conf->DMA_Streamx);
    LL_SPI_EnableIT_EOT(DAC_conf->SPIx);
}

//__attribute__((section("sram_func")))
void DAC_Device::write(float output)
{
    while (busy)
    {
    }

    busy = true;

    output = std::min(max_output, std::max(output, min_output));
    last_output = output;

    int32_t int_output = (int32_t)((output - zero_voltage) * inv_step_size);

    if (invert)
        int_output = -int_output;

    // Doesn't check that value is 20bit, may get unexpected results
    dma_buffer[0] = 0b00010000 + ((int_output >> 14) & 0x0f);
    dma_buffer[1] = (int_output >> 6) & 0xff;
    dma_buffer[2] = int_output & 0xff;
}

void DAC1_Device::write(float output)
{
    DAC_Device::write(output);
    begin_dma_transfer();
}
void DAC2_Device::write(float output)
{
    DAC_Device::write(output);
    begin_dma_transfer();
}
//__attribute__((section("sram_func")))
void DAC1_Device::dma_transmission_callback()
{
    while (!LL_SPI_IsActiveFlag_TXC(DAC_conf->SPIx))
    {
    }
    DisableChannel(DAC_conf->BDMA_Channelx);
    DAC_conf->bdma_clr_flag(DAC_conf->BDMAx);
    LL_SPI_ClearFlag_EOT(DAC_conf->SPIx);
    CLEAR_BIT(DAC_conf->SPIx->CR1, SPI_CR1_SPE);
    SET_BIT(DAC_conf->SPIx->IFCR, SPI_IFCR_TXTFC);
    while (LL_SPI_IsEnabled(DAC_conf->SPIx))
    {
    }
    CLEAR_BIT(DAC_conf->SPIx->CFG1, SPI_CFG1_TXDMAEN);

    busy = false;
}

void DAC2_Device::dma_transmission_callback()
{
    while (!LL_SPI_IsActiveFlag_TXC(DAC_conf->SPIx))
    {
    }
    DAC_conf->dma_clr_flag(DAC_conf->DMAx);
    LL_SPI_ClearFlag_EOT(DAC_conf->SPIx);
    CLEAR_BIT(DAC_conf->SPIx->CR1, SPI_CR1_SPE);
    SET_BIT(DAC_conf->SPIx->IFCR, SPI_IFCR_TXTFC);
    while (LL_SPI_IsEnabled(DAC_conf->SPIx))
    {
    }
    CLEAR_BIT(DAC_conf->SPIx->CFG1, SPI_CFG1_TXDMAEN);

    busy = false;
}

void DAC_Device::config_output()
{
    /*
     * The STM32 on-chip ADC (not the ADC_Device) is used to detect jumper configuration.
     * Based on this the output range of the DAC_Device is set
     */

    // read ADC value of lower voltage
    ADC_ChannelConfTypeDef adc_config = {0};
    adc_config.Channel = DAC_conf->SENL;
    adc_config.Rank = 1;
    adc_config.SamplingTime = ADC3_SAMPLETIME_2CYCLES_5;

    HAL_ADC_ConfigChannel(DAC_conf->STM_ADC, &adc_config);
    HAL_ADC_Start(DAC_conf->STM_ADC);
    HAL_ADC_PollForConversion(DAC_conf->STM_ADC, 1);

    uint32_t adc_result = HAL_ADC_GetValue(DAC_conf->STM_ADC);

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
    adc_config.Channel = DAC_conf->SENH;
    HAL_ADC_ConfigChannel(DAC_conf->STM_ADC, &adc_config);
    HAL_ADC_Start(DAC_conf->STM_ADC);
    HAL_ADC_PollForConversion(DAC_conf->STM_ADC, 1);

    adc_result = HAL_ADC_GetValue(DAC_conf->STM_ADC);

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
}

void DAC1_Device::config_output()
{
    DAC_Device::config_output();
    DAC_Device::prepare_buffer();
    begin_dma_transfer();

    // Wait till configuration is sent
    while (busy)
        ;
}
void DAC2_Device::config_output()
{
    DAC_Device::config_output();
    DAC_Device::prepare_buffer();
    begin_dma_transfer();

    // Wait till configuration is sent
    while (busy)
        ;
}
void DAC_Device::prepare_buffer()
{
    busy = true;

    // bring SYNC line low to prepare DAC

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
}

void DAC_Device::begin_dma_transfer()
{
    EnableChannel(DAC_conf->DMA_Streamx);
    LL_SPI_EnableDMAReq_TX(DAC_conf->SPIx);
    while (!IsEnabledChannel(DAC_conf->DMA_Streamx))
    {
    }
    ATOMIC_SET_BIT(DAC_conf->SPIx->CR1, SPI_CR1_SPE);
    SET_BIT(DAC_conf->SPIx->CR1, SPI_CR1_CSTART);
}
// __attribute__((section("sram_func")))
void DAC1_Device::begin_dma_transfer()
{
    LL_SPI_SetTransferSize(DAC_conf->SPIx, 3);
    SetDataLength(DAC_conf->BDMA_Channelx, 3);
    EnableChannel(DAC_conf->BDMA_Channelx);
    while (!IsEnabledChannel(DAC_conf->BDMA_Channelx))
    {
    }
    LL_SPI_EnableDMAReq_TX(DAC_conf->SPIx);
    while (!LL_SPI_IsEnabledDMAReq_TX(DAC_conf->SPIx))
    {
    }
    DAC_conf->SPIx->CR1 |= SPI_CR1_SPE;
    while (!LL_SPI_IsEnabled(DAC_conf->SPIx))
    {
    }
    DAC_conf->SPIx->CR1 |= SPI_CR1_CSTART;
}
// __attribute__((section("sram_func")))
void DAC2_Device::begin_dma_transfer()
{
    LL_SPI_SetTransferSize(DAC_conf->SPIx, 3);
    SetDataLength(DAC_conf->DMA_Streamx, 3);
    EnableChannel(DAC_conf->DMA_Streamx);
    while (!IsEnabledChannel(DAC_conf->DMA_Streamx))
    {
    }
    LL_SPI_EnableDMAReq_TX(DAC_conf->SPIx);
    while (!LL_SPI_IsEnabledDMAReq_TX(DAC_conf->SPIx))
    {
    }
    DAC_conf->SPIx->CR1 |= SPI_CR1_SPE;
    while (!LL_SPI_IsEnabled(DAC_conf->SPIx))
    {
    }
    DAC_conf->SPIx->CR1 |= SPI_CR1_CSTART;
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
