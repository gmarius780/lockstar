/*
 * adc_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

// ADC_RX_DMA_STREAM RX
// ADC_TX_DMA_STREAM TX
#include "ADCDevice.hpp"
#include "adc_config.h"
__attribute__((section(".data"))) uint8_t dmaADC_buffer[3] = {0};

ADC_Device::ADC_Device(ADC_Device_TypeDef *ADC_conf)
{
    this->ADC_conf = ADC_conf;
    single_channel_mode = false;
    if (ADC_conf->channel1_config == ADC_OFF || ADC_conf->channel2_config == ADC_OFF)
        single_channel_mode = true;

    adc_config_buffer = new uint8_t[DATAWIDTH]();
    ADCdma_buffer = dmaADC_buffer;

    channel1 = new ADC_Device_Channel(this, 1, ADC_conf->channel1_config);
    channel2 = new ADC_Device_Channel(this, 2, ADC_conf->channel2_config);

    busy = false;

    /*
     * Calculate the ADC configuration based on channel configurations
     * This is send at every conversion to the ADC device
     */
    volatile uint8_t code = 0b11111100; // TODO: magic number.. nice!
    code = ((code & 0b00011111) | (channel1->get_channel_code() << 5));
    code = ((code & 0b11100011) | (channel2->get_channel_code() << 2));
    adc_config_buffer[0] = code;

    // Setup perhipherals
    spi_handler = new SPI(ADC_conf->SPIx);
    LL_SPI_SetFIFOThreshold(ADC_conf->SPIx, LL_SPI_FIFO_TH_03DATA);

    ADC_conf->DMA_InitStructRx->PeriphOrM2MSrcAddress = (uint32_t) & (ADC_conf->SPIx->RXDR);
    ADC_conf->DMA_InitStructRx->MemoryOrM2MDstAddress = (uint32_t)ADCdma_buffer;

    ADC_conf->DMA_InitStructTx->PeriphOrM2MSrcAddress = (uint32_t) & (ADC_conf->SPIx->TXDR);
    ADC_conf->DMA_InitStructTx->MemoryOrM2MDstAddress = (uint32_t)adc_config_buffer;

    LL_DMA_Init(ADC_conf->DMARx, __LL_DMA_GET_STREAM(ADC_conf->DMA_StreamRx), ADC_conf->DMA_InitStructRx);
    LL_DMA_Init(ADC_conf->DMATx, __LL_DMA_GET_STREAM(ADC_conf->DMA_StreamTx), ADC_conf->DMA_InitStructTx);

    EnableIT_TC(ADC_conf->DMA_StreamRx);
}

ADC_Device_Channel::ADC_Device_Channel(ADC_Device *parentDevice, uint16_t channelID, uint8_t config)
{
    this->parent_device = parentDevice;
    this->channel_id = channelID;
    this->result = 0;

    /* from the LTC2353-16 Datasheet:
     * SoftSpan 111     +/- 10.24 V
     *          101     0 to 10.24 V
     *          011     +/- 5.12 V
     *          001     0 to 5.12 V
     *          000     channel off
     */

    switch (config)
    {
    case ADC_BIPOLAR_10V:
        channel_code = 0b111;
        this->step_size = 20.48f / 0xffff;
        this->two_comp = true;
        break;
    case ADC_BIPOLAR_5V:
        channel_code = 0b011;
        this->step_size = 10.24f / 0xffff;
        this->two_comp = true;
        break;
    case ADC_UNIPOLAR_10V:
        channel_code = 0b101;
        this->step_size = 10.24f / 0xffff;
        this->two_comp = false;
        break;
    case ADC_UNIPOLAR_5V:
        channel_code = 0b001;
        this->step_size = 5.12f / 0xffff;
        this->two_comp = false;
        break;
    default:
        channel_code = 0b000;
        this->step_size = 0.0f;
        this->two_comp = false;
    }
}

void ADC_Device_Channel::update_result(int16_t result)
{
    this->result = two_comp ? (step_size * result) : (step_size * (uint16_t)result);
}

void ADC_Device::start_conversion()
{

    while(busy){
    }

    // busy flag gets reset when DMA transfer is finished
    busy = true;
    

    SetDataLength(ADC_conf->DMA_StreamRx, DATAWIDTH);
    SetDataLength(ADC_conf->DMA_StreamTx, DATAWIDTH);

    EnableChannel(ADC_conf->DMA_StreamRx);
    LL_SPI_EnableDMAReq_RX(ADC_conf->SPIx);

    EnableChannel(ADC_conf->DMA_StreamTx);
    LL_SPI_EnableDMAReq_TX(ADC_conf->SPIx);

    ADC_conf->SPIx->CR1 |= SPI_CR1_SPE;
    ADC_conf->SPIx->CR1 |= SPI_CR1_CSTART;
}

void ADC_Device::spi_transmision_callback()
{
}

void ADC_Device::dma_transmission_callback(void)
{
    ADC_conf->dmaTx_clr_flag(ADC_conf->DMATx);
}
__attribute__((section("sram_func")))
void ADC_Device::dma_receive_callback(void)
{

    ADC_conf->dmaRx_clr_flag(ADC_conf->DMARx);
    ADC_conf->dmaTx_clr_flag(ADC_conf->DMATx);

    CLEAR_BIT(ADC_conf->SPIx->CR1, SPI_CR1_SPE);
    CLEAR_BIT(ADC_conf->SPIx->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

    ADC_conf->cnv_port->BSRR = ADC_conf->cnv_pin;
    ADC_conf->cnv_port->BSRR = ADC_conf->cnv_pin << 16U;
    channel2->update_result(bytes_to_u16(ADCdma_buffer[0], ADCdma_buffer[1]));
    channel1->update_result(bytes_to_u16(ADCdma_buffer[3], ADCdma_buffer[4]));
    busy = false;
}
