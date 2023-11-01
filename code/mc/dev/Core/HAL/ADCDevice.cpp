/*
 * adc_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

// ADC_RX_DMA_STREAM RX
// ADC_TX_DMA_STREAM TX
#include "ADCDevice.hpp"

ADC_Device::ADC_Device(uint8_t SPILane, uint8_t DMAStreamIn, uint8_t DMAChannelIn, uint8_t DMAStreamOut, uint8_t DMAChannelOut, GPIO_TypeDef *CNVPort, uint16_t CNVPin, uint8_t channel1Config, uint8_t channel2Config)
{
    this->cnv_port = CNVPort;
    this->cnv_pin = CNVPin;

    single_channel_mode = false;
    if (channel1Config == ADC_OFF || channel2Config == ADC_OFF)
        single_channel_mode = true;

    adc_config_buffer = new uint8_t[DATAWIDTH]();

    channel1 = new ADC_Device_Channel(this, 1, channel1Config);
    channel2 = new ADC_Device_Channel(this, 2, channel2Config);

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
    // cnv_port->BSRR = cnv_pin << 16;
    spi_handler = new SPI(ADC_SPI);
    // LL_SPI_SetFIFOThreshold(ADC_SPI, LL_SPI_FIFO_TH_03DATA);
    // LL_SPI_SetTransferSize(ADC_SPI, DATAWIDTH);
    // LL_SPI_SetReloadSize(ADC_SPI, DATAWIDTH);
    LL_SPI_SetMasterSSIdleness(ADC_SPI, LL_SPI_SS_IDLENESS_15CYCLE);
    // LL_SPI_SetInterDataIdleness(ADC_SPI, LL_SPI_ID_IDLENESS_01CYCLE);
    // LL_SPI_EnableIT_EOT(ADC_SPI);

    LL_DMA_InitTypeDef DMA_RX_InitStruct = {0};
    LL_DMA_InitTypeDef DMA_TX_InitStruct = {0};

    DMA_RX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi_handler->getRXDRAddress();
    DMA_RX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    DMA_RX_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_RX_InitStruct.Mode = ADC_DMA_MODE;
    DMA_RX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_RX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_RX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_RX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_RX_InitStruct.NbData = DATAWIDTH;
    DMA_RX_InitStruct.PeriphRequest = LL_DMAMUX1_REQ_ADC_SPI_RX;
    DMA_RX_InitStruct.Priority = LL_DMA_PRIORITY_HIGH;
    DMA_RX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
    DMA_RX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_RX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_RX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;

    DMA_TX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi_handler->getTXDRAddress();
    DMA_TX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)adc_config_buffer;
    DMA_TX_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_TX_InitStruct.Mode = ADC_DMA_MODE;
    DMA_TX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_TX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_TX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_TX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_TX_InitStruct.NbData = DATAWIDTH;
    DMA_TX_InitStruct.PeriphRequest = LL_DMAMUX1_REQ_ADC_SPI_TX;
    DMA_TX_InitStruct.Priority = LL_DMA_PRIORITY_MEDIUM;
    DMA_TX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
    DMA_TX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_TX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_TX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;

    dma_input_handler = new DMA(DMA1, ADC_DMA_RX_STREAM, &DMA_RX_InitStruct);
    dma_output_handler = new DMA(DMA1, ADC_DMA_TX_STREAM, &DMA_TX_InitStruct);

    // LL_DMA_SetM2MSrcAddress(DMA2, LL_DMA_STREAM_7, (uint32_t)dma_buffer);
    // LL_DMA_SetM2MDstAddress
    // LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_7);

#ifdef DOUBLE_BUFFER
    LL_DMA_SetMemory1Address(DMA1, ADC_DMA_RX_STREAM, (uint32_t)dma_buffer2);
    LL_DMA_EnableDoubleBufferMode(DMA1, ADC_DMA_RX_STREAM);
    // dma_output_handler->enable_tc_irq();
    // dma_input_handler->enable_tc_irq();
#endif

#ifndef DOUBLE_BUFFER
    dma_input_handler->enable_tc_irq();
    dma_output_handler->enable_tc_irq();
#endif
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

    LL_SPI_EnableDMAReq_RX(ADC_SPI);
    LL_DMA_EnableStream(DMA1, ADC_DMA_TX_STREAM);
    LL_DMA_EnableStream(DMA1, ADC_DMA_RX_STREAM);
    LL_SPI_EnableDMAReq_TX(ADC_SPI);
    while (!LL_DMA_IsEnabledStream(DMA1, ADC_DMA_TX_STREAM) && !LL_DMA_IsEnabledStream(DMA1, ADC_DMA_RX_STREAM))
    {
    }

    ATOMIC_SET_BIT(ADC_SPI->CR1, SPI_CR1_SPE);

    NSS_PORT->BSRR = NSS_PIN;
    NSS_PORT->BSRR = NSS_PIN << 16U;

    SET_BIT(ADC_SPI->CR1, SPI_CR1_CSTART);
}

void ADC_Device::spi_transmision_callback()
{
}

void ADC_Device::dma_transmission_callback(void)
{
    TX_DMA_ClearFlag(DMA1);
    ATOMIC_MODIFY_REG(ADC_TX_DMA_STREAM->NDTR, DMA_SxNDT, DATAWIDTH);
    // LL_DMA_DisableStream(DMA1, ADC_DMA_TX_STREAM);
}

void ADC_Device::dma_receive_callback(void)
{
    RX_DMA_ClearFlag(DMA1);
    if (sample++ == SAMPLES)
    {
        ATOMIC_CLEAR_BIT(ADC_SPI->CR1, SPI_CR1_SPE);
        return;
    }
#ifdef NORMAL_MODE
    while (LL_SPI_IsActiveFlag_RXWNE(ADC_SPI) || LL_SPI_GetRxFIFOPackingLevel(ADC_SPI))
    {
    }

    ATOMIC_CLEAR_BIT(ADC_SPI->CR1, SPI_CR1_SPE);

    ATOMIC_MODIFY_REG(ADC_RX_DMA_STREAM->NDTR, DMA_SxNDT, DATAWIDTH);

    NSS_PORT->BSRR = NSS_PIN;
    NSS_PORT->BSRR = NSS_PIN << 16U;

    ATOMIC_SET_BIT(ADC_TX_DMA_STREAM->CR, DMA_SxCR_EN);
    ATOMIC_SET_BIT(ADC_RX_DMA_STREAM->CR, DMA_SxCR_EN);
    ATOMIC_SET_BIT(ADC_SPI->CR1, SPI_CR1_SPE);

    // int16_t tempCH2 = bytes_to_u16(dma_buffer[0], dma_buffer[1]);
    // int16_t tempCH1 = bytes_to_u16(dma_buffer[3], dma_buffer[4]);

    // channel2->update_result(tempCH2);
    // channel1->update_result(tempCH1);

    SET_BIT(ADC_SPI->CR1, SPI_CR1_CSTART);

#endif
#ifdef DOUBLE_BUFFER
    int16_t *buffer;

    if (LL_DMA_GetCurrentTargetMem(DMA1, ADC_DMA_RX_STREAM) == LL_DMA_CURRENTTARGETMEM0)
    {
        buffer = (int16_t *)dma_buffer;
    }
    else
    {
        buffer = (int16_t *)dma_buffer2;
    }
    channel2->update_result(((int16_t)(buffer[0] << 8)) + ((int16_t)buffer[1]));
    channel1->update_result(((int16_t)(buffer[3] << 8)) + ((int16_t)buffer[4]));
#endif
}
