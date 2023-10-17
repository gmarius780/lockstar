/*
 * adc_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

// DMA1_Stream4 RX
// DMA1_Stream5 TX
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
    spi_handler = new SPI(SPI2);
    LL_SPI_SetMasterSSIdleness(SPI2, LL_SPI_SS_IDLENESS_10CYCLE);
    // LL_SPI_SetInterDataIdleness(SPI2, LL_SPI_ID_IDLENESS_10CYCLE);


    LL_DMA_InitTypeDef DMA_RX_InitStruct = {0};
    LL_DMA_InitTypeDef DMA_TX_InitStruct = {0};

    DMA_RX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi_handler->getRXDRAddress();
    DMA_RX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)dma_buffer;
    DMA_RX_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_RX_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_RX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_RX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_RX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_RX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_RX_InitStruct.NbData = DATAWIDTH;
    DMA_RX_InitStruct.PeriphRequest = LL_DMAMUX1_REQ_SPI2_RX;
    DMA_RX_InitStruct.Priority = LL_DMA_PRIORITY_MEDIUM;
    DMA_RX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_ENABLE;
    DMA_RX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_RX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_RX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;

    DMA_TX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi_handler->getTXDRAddress();
    DMA_TX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)adc_config_buffer;
    DMA_TX_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_TX_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_TX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_TX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_TX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_TX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_TX_InitStruct.NbData = DATAWIDTH;
    DMA_TX_InitStruct.PeriphRequest = LL_DMAMUX1_REQ_SPI2_TX;
    DMA_TX_InitStruct.Priority = LL_DMA_PRIORITY_HIGH;
    DMA_TX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_ENABLE;
    DMA_TX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_TX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_TX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;

    dma_input_handler = new DMA(DMA1, LL_DMA_STREAM_2, &DMA_RX_InitStruct);
    dma_output_handler = new DMA(DMA1, LL_DMA_STREAM_3, &DMA_TX_InitStruct);

    dma_input_handler->enable_tc_irq();
    
    // LL_SPI_EnableDMAReq_RX(SPI2);
    // LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_3);
    // LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
    // LL_SPI_EnableDMAReq_TX(SPI2);

    // spi_handler->enableSPI();
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
    if (busy)
        return;

    LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_3, DATAWIDTH);
    LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_2, DATAWIDTH);
    LL_SPI_EnableDMAReq_RX(SPI2);
    LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_3);
    LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
    LL_SPI_EnableDMAReq_TX(SPI2);

    spi_handler->enableSPI();

    // busy flag gets reset when DMA transfer is finished
    busy = true;

    // cnv_port->BSRR = cnv_pin;
    
    // volatile uint8_t delay = 0;
    // while (delay--){

    // }
    // cnv_port->BSRR = (uint32_t)cnv_pin << 16U;
    // delay = 20;
    // while (delay--){

    // }

    LL_SPI_StartMasterTransfer(SPI2);
}

__attribute__((section("sram_func"))) void ADC_Device::arm_dma()
{
    
}

__attribute__((section("sram_func"))) void ADC_Device::dma_receive_callback()
{
    SPI_DMA_EOT_Callback(SPI2);
}

__attribute__((section("sram_func"))) void ADC_Device::dma_transmission_callback()
{
        // 1. Disable TX Stream
    while (!LL_SPI_IsActiveFlag_TXC(SPIx))
    {
    }
    LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_3);
    
}


void ADC_Device::SPI_DMA_EOT_Callback(SPI_TypeDef *SPIx)
{

    // 2. Poll if RX FIFO empty
    while (LL_SPI_IsActiveFlag_RXWNE(SPIx) || LL_SPI_GetRxFIFOPackingLevel(SPIx))
    {
    }
    LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);

    while (LL_DMA_IsEnabledStream(DMA1, LL_DMA_STREAM_3) || LL_DMA_IsEnabledStream(DMA1, LL_DMA_STREAM_2))
    {
    }

    // LL_DMA_DisableIT_TC(DMA1, LL_DMA_STREAM_2);
    LL_SPI_Disable(SPIx);
    while (LL_SPI_IsEnabled(SPIx))
    {
    }
    LL_SPI_DisableDMAReq_TX(SPIx);
    LL_SPI_DisableDMAReq_RX(SPIx);
    channel2->update_result(((int16_t)(dma_buffer[0] << 8)) + ((int16_t)dma_buffer[1]));
    channel1->update_result(((int16_t)(dma_buffer[3] << 8)) + ((int16_t)dma_buffer[4]));
    busy = false;
    turn_LED1_on();
}
