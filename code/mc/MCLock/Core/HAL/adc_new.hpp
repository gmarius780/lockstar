/*
 * adc_new.hpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#ifndef HAL_ADC_NEW_HPP_
#define HAL_ADC_NEW_HPP_

#include "stm32f4xx_hal.h"
#include "dma_new.hpp"
#include "spi.hpp"

#define ADC_BIPOLAR_10V     (uint8_t)0b111
#define ADC_BIPOLAR_5V      (uint8_t)0b011
#define ADC_UNIPOLAR_10V    (uint8_t)0b101
#define ADC_UNIPOLAR_5V     (uint8_t)0b001
#define ADC_OFF             (uint8_t)0b000

#define DATAWIDTH           6
#define BUFFERSIZE          DATAWIDTH*10

class ADC_Device;

class ADC_Device_Channel {
public:
    ADC_Device_Channel(ADC_Device* parentDevice, uint16_t ChannelId, uint8_t setup);
    float getResult() { return result; };
    uint8_t getChannelCode() { return channelCode; };

private:
    void updateResult();
    volatile float result;
    float stepSize;
    bool twoComp;
    uint8_t ChannelId, channelCode;
};

class ADC_Device {
public:
    ADC_Device(uint8_t SPILane, 
                uint8_t DMA_Stream_In, 
                uint8_t DMA_Channel_In, 
                uint8_t DMA_Stream_Out, 
                uint8_t DMA_Channel_Out, 
                GPIO_TypeDef* CNV_Port, 
                uint16_t CNV_Pin, 
                uint8_t Channel1Config, 
                uint8_t Channel2Config);
    ADC_Device_Channel *channel1, *channel2;
    void startConversion();
    void DMATransmissionCallback();
    void clearBuffer();
    volatile uint8_t* getDataBuffer() { return dataBuffer; }

private:
    void armDMA();
    void disarmDMA();

    volatile uint8_t* dataBuffer;
    volatile uint8_t* ADC_configBuffer;

    uint16_t CNVPin;
    GPIO_TypeDef* CNVPort;

    bool singleChannelMode;

    DMA *DMAInputHandler, *DMAOutputHandler;
    SPI *SPIHandler;
    DMA_config_t DMAInConfig, DMAOutConfig;
};



#endif /* HAL_ADC_NEW_HPP_ */
