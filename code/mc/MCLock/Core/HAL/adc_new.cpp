/*
 * adc_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#include "adc_new.hpp"

ADC_Device::ADC_Device(uint8_t SPILane, uint8_t DMAStreamIn, uint8_t DMAChannelIn, uint8_t DMAStreamOut, uint8_t DMAChannelOut, GPIO_TypeDef* CNVPort, uint16_t CNVPin, uint8_t channel1Config, uint8_t channel2Config, uint8_t bufferSize) {
    
    // Setup of the relevant datastructures
    this->bufferSize = bufferSize;
    dataBuffer      = new uint8_t[bufferSize]();
    this->CNVPort   = CNVPort;
    this->CNVPin    = CNVPin;

    singleChannelMode = false;
    if(channel1Config == ADC_OFF || channel2Config == ADC_OFF)
        singleChannelMode = true;

    ADC_configBuffer = new uint8_t[DATAWIDTH]();

    // Initialize the two channels of the ADC Device
    channel1 = new ADC_Device_Channel(this, 1, channel1Config);
    channel2 = new ADC_Device_Channel(this, 2, channel2Config);

    /*
    * Calculate the ADC configuration based on channel configurations
    * This is send at every conversion to the ADC device
    */
    volatile uint8_t code = 0b11111100; //TODO: magic number.. nice!
    code = ((code&0b00011111) | (channel1->getChannelCode()<<5));
    code = ((code&0b11100011) | (channel2->getChannelCode()<<2));
    ADC_configBuffer[0] = code;

    // Setup perhipherals
    SPIHandler = new SPI(SPILane);

    uint8_t DMAprio = 2;
    DMAInConfig.priority = DMAprio;
    DMAInConfig.CR = 0;
    // reset 3 bits that define channel
    DMAInConfig.CR &= ~(DMA_SxCR_CHSEL);
    // set channel via 3 control bits
    DMAInConfig.CR |= DMAChannelIn * DMA_SxCR_CHSEL_0; 
    // set stream priority from very low (00) to very high (11)
    DMAInConfig.CR &= ~(DMA_SxCR_PL); 
    // reset 2 bits that define priority
    DMAInConfig.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMAInConfig.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMAInConfig.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "peripheral to memory"
    DMAInConfig.CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
    // Clear DBM bit
    DMAInConfig.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMAInConfig.CR  |= DMA_SxCR_TCIE;

    DMAInConfig.stream      = DMAStreamIn;
    DMAInConfig.channel     = DMAChannelIn;
    DMAInConfig.PAR         = (uint32_t)SPIHandler->getDRAddress();
    DMAInConfig.M0AR        = (uint32_t)dataBuffer;
    DMAInConfig.M1AR		= 0;
    DMAInConfig.NDTR        = 0;
    DMAInputHandler         = new DMA(DMAInConfig);

    DMAOutConfig.priority = DMAprio;
    DMAOutConfig.CR = 0;
    DMAOutConfig.CR &= ~(DMA_SxCR_CHSEL); // reset 3 bits that define channel
    DMAOutConfig.CR |= DMAChannelOut * DMA_SxCR_CHSEL_0; // set channel via 3 control bits
    // set stream priority from very low (00) to very high (11)
    DMAOutConfig.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    DMAOutConfig.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMAOutConfig.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMAOutConfig.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "memory to peripheral"
    DMAOutConfig.CR &= ~DMA_SxCR_DIR_1;
    DMAOutConfig.CR |= DMA_SxCR_DIR_0;
    // Clear DBM bit
    DMAOutConfig.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Disable transmission-complete interrupt
    DMAOutConfig.CR  &= ~DMA_SxCR_TCIE;

    DMAOutConfig.stream     = DMAStreamOut;
    DMAOutConfig.channel    = DMAChannelOut;
    DMAOutConfig.PAR        = (uint32_t)SPIHandler->getDRAddress();
    DMAOutConfig.M0AR       = (uint32_t)dataBuffer;
    DMAOutConfig.M1AR		= 0;
    DMAOutConfig.NDTR       = 0;
    DMAOutputHandler        = new DMA(DMAOutConfig);

    SPIHandler->bindDMAHandlers(DMAOutputHandler, DMAInputHandler);
    SPIHandler->enableSPI();
}

ADC_Device_Channel::ADC_Device_Channel(ADC_Device* parentDevice, uint16_t channelID, uint8_t config){
    this->parentDevice = parentDevice;
    this->channelID = channelID;
    this->result = 0;

    /* from the LTC2353-16 Datasheet:
     * SoftSpan 111     +/- 10.24 V
     *          101     0 to 10.24 V
     *          011     +/- 5.12 V
     *          001     0 to 5.12 V
     *          000     channel off
     */

    switch(config){
    case ADC_BIPOLAR_10V:
        channelCode = 0b111;
        this->stepSize = 20.48f / 0xffff;
        this->twoComp = true;
        break;
    case ADC_BIPOLAR_5V:
        channelCode = 0b011;
        this->stepSize = 10.24f / 0xffff;
        this->twoComp = true;
        break;
    case ADC_UNIPOLAR_10V:
        channelCode = 0b101;
        this->stepSize = 10.24f / 0xffff;
        this->twoComp = false;
        break;
    case ADC_UNIPOLAR_5V:
        channelCode = 0b001;
        this->stepSize = 5.12f / 0xffff;
        this->twoComp = false;
        break;
    default:
        channelCode = 0b000;
        this->stepSize = 0.0f;
        this->twoComp = false;
    }
}

__attribute__((section("sram_func")))
void ADC_Device_Channel::updateResult(int16_t result) {
    // convert to float
    this->result = twoComp ? (stepSize *  result) : (stepSize * (uint16_t)result);
    // TODO: implement lowpass
}

__attribute__((section("sram_func")))
float ADC_Device_Channel::getResult() { return result; }

__attribute__((section("sram_func")))
void ADC_Device::startConversion() {

    CNVPort->BSRR = CNVPin;
    volatile uint8_t delay = 0;
    while(delay--);
    CNVPort->BSRR = (uint32_t)CNVPin << 16U;
    delay = 5;
    while(delay--);
    //M: sleep?

    armDMA();
}

__attribute__((section("sram_func")))
void ADC_Device::armDMA() {
    DMAOutputHandler->setMemoryAddress(ADC_configBuffer,0);
    DMAOutputHandler->setNumberOfData(6);
    
    DMAInputHandler->setMemoryAddress(dataBuffer,0);
    DMAInputHandler->setNumberOfData(6);

    DMAOutputHandler->enableDMA();
    DMAInputHandler->enableDMA();

    SPIHandler->enableSPI_DMA();   
}

__attribute__((section("sram_func")))
void ADC_Device::DMATransmissionCallback() {
    SPIHandler->disableSPI_DMA();
    DMAInputHandler->resetTransferCompleteInterruptFlag();
    DMAOutputHandler->resetTransferCompleteInterruptFlag();

    channel2->updateResult(((int16_t)(dataBuffer[0] << 8)) + ((int16_t)dataBuffer[1]));
    channel1->updateResult(((int16_t)(dataBuffer[3] << 8)) + ((int16_t)dataBuffer[4]));
}

