/*
 * dac.h
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#ifndef ADC_H_
#define ADC_H_

#include "stm32f4xx_hal.h"

#include "../Lib/oscilloscope.hpp"
#include "dma.hpp"
#include "flashmemory.hpp"

#define ADC_BIPOLAR_10V		(uint8_t)0b111
#define ADC_BIPOLAR_5V		(uint8_t)0b011
#define ADC_UNIPOLAR_10V	(uint8_t)0b101
#define ADC_UNIPOLAR_5V		(uint8_t)0b001
#define ADC_OFF				(uint8_t)0b000

class ADC_Dev;

class ADC_Channel
{
	public:
		ADC_Channel(ADC_Dev* ParentDevice, uint16_t ChannelId);
		float GetFloat();
		int16_t GetInt();
		void SetInput(int16_t input);
		void Setup(uint8_t config);
		void SetLowPass(bool LowPassOn, float Weight);
		void ResetLowPass();
		uint8_t GetConfig(){ return config; };

		friend class ADC_Dev;
		friend class Oscilloscope;
		friend class UserData;

	private:
		ADC_Dev* ParentDevice;
		float StepSize;
		bool TwoComp;
		volatile int16_t input;
		volatile float InputFloat;
		uint8_t ChannelId, config;
		bool LowPassOn, LowPassReset;
		float LowPassWeight, LowPassPrev;
};

class ADC_Dev
{
	public:
		ADC_Channel *Channel1, *Channel2;
		ADC_Dev(uint8_t SPI, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, GPIO_TypeDef* CNV_Port, uint16_t CNV_Pin, bool scanmode=false);
		void Read();
		void Callback();
		bool isReady() { return ready; };
		bool isZero() { return (Buffer[0]==0) && (Buffer[1]==0) && (Buffer[2]==0) && (Buffer[3]==0) && (Buffer[4]==0) && (Buffer[5]==0); };
		uint8_t* getBuffer(){return Buffer;};
		void startScanmode();
		void SPI_byteTransmitted();
		void DMA_TX_Callback();
		void startTransmission();

		friend class Oscilloscope;
		friend class ADC_Channel;
		friend class UserData;

	private:
		GPIO_TypeDef* CNV_Port;
		uint16_t CNV_Pin;
		uint8_t* Buffer;
		uint8_t* Softspan;
		volatile bool ready;
		SPI_DMA_Handler* DMAHandler;
		volatile uint8_t BufferSize;
		void UpdateSoftSpan(uint8_t chcode, uint8_t ChannelId);

		bool scanmode;
		int transmittedBytes;
		int scanmodeReadoutPointer;
};

#endif /* ADC_H_ */
