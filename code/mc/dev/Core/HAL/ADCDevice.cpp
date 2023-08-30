/*
 * adc_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#include "ADCDevice.hpp"

ADC_Device::ADC_Device(uint8_t SPILane, uint8_t DMAStreamIn, uint8_t DMAChannelIn, uint8_t DMAStreamOut, uint8_t DMAChannelOut, GPIO_TypeDef* CNVPort, uint16_t CNVPin, uint8_t channel1Config, uint8_t channel2Config) {
    
    dma_buffer      = new uint8_t[DATAWIDTH]();
    this->cnv_port   = CNVPort;
    this->cnv_pin    = CNVPin;

    single_channel_mode = false;
    if(channel1Config == ADC_OFF || channel2Config == ADC_OFF)
        single_channel_mode = true;

    adc_config_buffer = new uint8_t[DATAWIDTH]();

    channel1 = new ADC_Device_Channel(this, 1, channel1Config);
    channel2 = new ADC_Device_Channel(this, 2, channel2Config);

    busy = false;

    /*
    * Calculate the ADC configuration based on channel configurations
    * This is send at every conversion to the ADC device
    */
    volatile uint8_t code = 0b11111100; //TODO: magic number.. nice!
    code = ((code&0b00011111) | (channel1->get_channel_code()<<5));
    code = ((code&0b11100011) | (channel2->get_channel_code()<<2));
    adc_config_buffer[0] = code;

    // Setup perhipherals
    spi_handler = new SPI(SPILane);

    uint8_t DMAprio = 2;
    dma_in_config.priority = DMAprio;
    dma_in_config.CR = 0;
    // reset 3 bits that define channel
    dma_in_config.CR &= ~(DMA_SxCR_PL);
    // set channel via 3 control bits
    dma_in_config.CR |= DMAChannelIn * DMA_SxCR_PL; 
    // set stream priority from very low (00) to very high (11)
    dma_in_config.CR &= ~(DMA_SxCR_PL); 
    // reset 2 bits that define priority
    dma_in_config.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    dma_in_config.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    dma_in_config.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "peripheral to memory"
    dma_in_config.CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
    // Clear DBM bit
    dma_in_config.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    dma_in_config.CR  |= DMA_SxCR_TCIE;

    dma_in_config.stream      = DMAStreamIn;
    dma_in_config.channel     = DMAChannelIn;
    dma_in_config.PAR         = (uint32_t)spi_handler->getDRAddress();
    dma_in_config.M0AR        = (uint32_t)dma_buffer;
    dma_in_config.M1AR		= 0;
    dma_in_config.NDTR        = 0;

    dma_input_handler         = new DMA(dma_in_config);

    dma_out_config.priority = DMAprio;
    dma_out_config.CR = 0;
    dma_out_config.CR &= ~(DMA_SxCR_PL); // reset 3 bits that define channel
    dma_out_config.CR |= DMAChannelOut * DMA_SxCR_PL; // set channel via 3 control bits
    // set stream priority from very low (00) to very high (11)
    dma_out_config.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    dma_out_config.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    dma_out_config.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    dma_out_config.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "memory to peripheral"
    dma_out_config.CR &= ~DMA_SxCR_DIR_1;
    dma_out_config.CR |= DMA_SxCR_DIR_0;
    // Clear DBM bit
    dma_out_config.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Disable transmission-complete interrupt
    dma_out_config.CR  &= ~DMA_SxCR_TCIE;

    dma_out_config.stream     = DMAStreamOut;
    dma_out_config.channel    = DMAChannelOut;
    dma_out_config.PAR        = (uint32_t)spi_handler->getDRAddress();
    dma_out_config.M0AR       = (uint32_t)dma_buffer;
    dma_out_config.M1AR		= 0;
    dma_out_config.NDTR       = 0;

    dma_output_handler        = new DMA(dma_out_config);

    spi_handler->enableSPI();
}

ADC_Device_Channel::ADC_Device_Channel(ADC_Device* parentDevice, uint16_t channelID, uint8_t config){
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

    switch(config) {
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

__attribute__((section("sram_func")))
void ADC_Device_Channel::update_result(int16_t result) {
	this->result = two_comp ? (step_size*result) : (step_size*(uint16_t)result);
}

__attribute__((section("sram_func")))
void ADC_Device::start_conversion() {
	if(busy)
		return;

	// busy flag gets reset when DMA transfer is finished
	busy = true;

    cnv_port->BSRR = cnv_pin;
    volatile uint8_t delay = 0;
    while(delay--);
    cnv_port->BSRR = (uint32_t)cnv_pin << 16U;
    delay = 5;
    while(delay--);

    arm_dma();
}

__attribute__((section("sram_func")))
void ADC_Device::arm_dma() {
	dma_output_handler->disableDMA();
	dma_input_handler->disableDMA();

    // TODO: create a arm_dma method in the DMA class
    dma_output_handler->setMemory0Address(adc_config_buffer);
    dma_output_handler->setNumberOfData(6);
    
    dma_input_handler->setMemory0Address(dma_buffer);
    dma_input_handler->setNumberOfData(6);

    dma_output_handler->enableDMA();
    dma_input_handler->enableDMA();

    spi_handler->enableSPI_DMA();
}

__attribute__((section("sram_func")))
void ADC_Device::dma_transmission_callback() {
    spi_handler->disableSPI_DMA();
    dma_input_handler->resetTransferCompleteInterruptFlag();
    dma_output_handler->resetTransferCompleteInterruptFlag();

    channel2->update_result(((int16_t)(dma_buffer[0] << 8)) + ((int16_t)dma_buffer[1]));
    channel1->update_result(((int16_t)(dma_buffer[3] << 8)) + ((int16_t)dma_buffer[4]));

    busy = false;
}

