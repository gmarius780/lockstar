/*
 * dac.cpp
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#include "adc.hpp"

#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal_spi.h"

#include "leds.hpp"

#define TIMEOUT				1000000
#define SCANBUFFER_SIZE 	10

#include "main.h"

ADC_Dev::ADC_Dev(uint8_t SPI, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, GPIO_TypeDef* CNV_Port, uint16_t CNV_Pin, bool scanmode)
{
	this->CNV_Port = CNV_Port;
	this->CNV_Pin = CNV_Pin;

	this->scanmode = scanmode;
	this->scanmodeReadoutPointer = (SCANBUFFER_SIZE-1)*6;
	// One ADC conversion takes 6 bytes,
	// so if the scan buffer should store SCANBUFFER_SIZE
	// conversions, the buffer has to to be 6*SCANBUFFER_SIZE bytes.

	if(scanmode)
		this->Buffer = new uint8_t[6*SCANBUFFER_SIZE]();
	 else
		this->Buffer = new uint8_t[6];

	this->Softspan = new uint8_t[6]();
	this->ready = true;

	this->Channel1 = new ADC_Channel(this, 1);
	this->Channel1->Setup(ADC_BIPOLAR_10V);
	this->Channel2 = new ADC_Channel(this, 2);
	this->Channel2->Setup(ADC_BIPOLAR_10V);

	this->Softspan[0] = 0b11111100;

	this->DMAHandler = new SPI_DMA_Handler(SPI, DMA_Stream_In, DMA_Channel_In, DMA_Stream_Out, DMA_Channel_Out, 2);
	if(scanmode) {
		SPI_DMA_Handler::enableCircularMode(DMAHandler->getInputStream());
		SPI_DMA_Handler::enableCircularMode(DMAHandler->getOutputStream());
	}
}

void ADC_Dev::startScanmode() {

	if(!scanmode)
		return;

	//SPI_DMA_Handler::setupDMAStream(DMAHandler->getOutputStream(), Softspan, 6);
	DMA_Stream_TypeDef* DMA_Out = DMAHandler->getOutputStream();
	DMA_Out->M0AR = (uint32_t)Softspan;
	// set number of data items
	DMA_Out->NDTR = 6;
	// activate stream
	DMA_Out->CR |= DMA_SxCR_EN;


	//SPI_DMA_Handler::setupDMAStream(DMAHandler->getInputStream(), Buffer, BufferSize);
	DMA_Stream_TypeDef* DMA_In = DMAHandler->getInputStream();
	DMA_In->M0AR = (uint32_t)Buffer;
	// set number of data items
	DMA_In->NDTR = BufferSize;
	// activate stream
	DMA_In->CR |= DMA_SxCR_EN;


	// TODO: redo this whole buffer stuff.. got unreadable with this additional scan mode

	// Tell ADC to start conversion
	CNV_Port->BSRR = CNV_Pin;
	CNV_Port->BSRR = (uint32_t)CNV_Pin << 16U;
	// Wait for conversion, enable counter
	TIM4->CR1 |= TIM_CR1_CEN;
}

void ADC_Dev::startTransmission() {
	// Disable Counter
	TIM4->CR1 &= ~(TIM_CR1_CEN);
	// Turn DMA back on
	SPI1->CR2 |= (SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
	// Clear the interrupt flag
	TIM4->SR &= ~(TIM_SR_UIF);
}

void ADC_Dev::DMA_TX_Callback() {
	if(!scanmode) {
		// clear input stream DMA interrupt flags
		DMA2->LIFCR |= (1<<27 | 1<<26 | 1<<25);
		return;
	}

	// Disable SPI_DMA
	SPI1->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

	// clear input stream DMA interrupt flags
	DMA2->LIFCR = (1<<27 | 1<<26 | 1<<25);

	// wait while SPI is busy
	while(SPI1->SR & SPI_SR_BSY);

	if(scanmodeReadoutPointer == (SCANBUFFER_SIZE-1)*6)
		scanmodeReadoutPointer = 0;
	else
		scanmodeReadoutPointer += 6;

	// Tell ADC to start conversion
	CNV_Port->BSRR = CNV_Pin;
	volatile int32_t delay = 0;//0,1
	while (delay--);
	CNV_Port->BSRR = (uint32_t)CNV_Pin << 16U;
	// Wait for conversion, enable timer
	TIM4->CR1 |= TIM_CR1_CEN;

	// Note: first byte softspan: 0xe0

}

__attribute__((section("sram_func")))
void ADC_Dev::Read()
{
	if(scanmode) {
		Channel2->SetInput(( (int16_t)Buffer[scanmodeReadoutPointer+0] << 8) + Buffer[scanmodeReadoutPointer+1]);
		Channel1->SetInput(( (int16_t)Buffer[scanmodeReadoutPointer+3] << 8) + Buffer[scanmodeReadoutPointer+4]);
		return;
	}

	// device busy?
	if(!ready)
		return;

	// mark device as busy
	ready = false;

	// read SPI data register to clear it
	//SPI->DR;

	// Start conversion by pushing CNV high
	CNV_Port->BSRR = CNV_Pin;
	volatile int32_t delay = 0;//0,1
	while (delay--);
	CNV_Port->BSRR = (uint32_t)CNV_Pin << 16U;
	if (true/*BufferSize==6*/) {
		delay = 5;// 5,3
		while (delay--);
	}
	
	DMAHandler->Transfer(Buffer, Softspan, BufferSize);

}

__attribute__((section("sram_func")))
void ADC_Dev::Callback()
{
	if(scanmode) {
		// clear the interrupt flags
		DMA2->LIFCR = (1<<21 | 1<<20 | 1<<19);
		return;
	}

	DMAHandler->Callback();
	Channel2->SetInput(( (int16_t)Buffer[0] << 8) + Buffer[1]);
	Channel1->SetInput(( (int16_t)Buffer[3] << 8) + Buffer[4]);

	// mark device as ready
	ready = true;
}




void ADC_Dev::UpdateSoftSpan(uint8_t chcode, uint8_t ChannelId)
{
	/* from Datasheet:
	 * SoftSpan programmed to device is given as S2[2] S2[1] S2[0] S1[2] S1[1] S1[0] . . . . . . . . . . . . . .
	 * Note that Hardware Device 2 is Input Channel 1.
	 */

	// TODO: make this function more readable for scan mode (maybe export to separate function for scanmode...)

	volatile uint8_t code = Softspan[0];
	if(ChannelId==1)
		code = ((code&0b00011111) | (chcode<<5));
	else
		code = ((code&0b11100011) | (chcode<<2));

	// if Channel 1 is off, we can shorten the communication protocol and just read 24bits for Channel 1
	if(!(code & 0b11100000))  // Channel 1 deactivated
		BufferSize = 3;
	else
		BufferSize = 6;
	// setup buffers accordingly
	if(scanmode) { // TODO: in scan mode setup buffer for 1 channel only as well.
		BufferSize = 6*SCANBUFFER_SIZE;
		delete this->Softspan;
		Softspan = new uint8_t[6];
		Softspan[0] = code;
	} else {
		delete Buffer;
		Buffer = new uint8_t[BufferSize];
		delete this->Softspan;
		Softspan = new uint8_t[BufferSize];
		Softspan[0] = code;
	}

}


ADC_Channel::ADC_Channel(ADC_Dev* ParentDevice, uint16_t ChannelId){
	this->ParentDevice = ParentDevice;
	this->ChannelId = ChannelId;

	this->LowPassOn = false;
}

void ADC_Channel::Setup(uint8_t config)
{
	/* from the LTC2353-16 Datasheet:
	 * SoftSpan 111		+/- 10.24 V
	 * 			101 	0 to 10.24 V
	 * 			011		+/- 5.12 V
	 * 			001		0 to 5.12 V
	 * 			000		channel off
	 */
	int8_t chcode;

	this->config = config;

	switch(config){
	case ADC_BIPOLAR_10V:
		chcode = 0b111;
		this->StepSize = 20.48f / 0xffff;
		this->TwoComp = true;
		break;
	case ADC_BIPOLAR_5V:
		chcode = 0b011;
		this->StepSize = 10.24f / 0xffff;
		this->TwoComp = true;
		break;
	case ADC_UNIPOLAR_10V:
		chcode = 0b101;
		this->StepSize = 10.24f / 0xffff;
		this->TwoComp = false;
		break;
	case ADC_UNIPOLAR_5V:
		chcode = 0b001;
		this->StepSize = 5.12f / 0xffff;
		this->TwoComp = false;
		break;
	default:
		chcode = 0b000;
		this->StepSize = 0.0f;
		this->TwoComp = false;
	}

	this->ParentDevice->UpdateSoftSpan(chcode, this->ChannelId);
}

__attribute__((section("sram_func")))
void ADC_Channel::SetInput(int16_t input)
{
	this->input = input;
	// convert to float
	InputFloat = TwoComp ? (StepSize *  GetInt()) : (StepSize * (uint16_t)GetInt());
	// low pass
	if(LowPassOn) {
		if(LowPassReset)
			LowPassReset = false;
		else {
			InputFloat = LowPassWeight * LowPassPrev + (1.0f - LowPassWeight) * InputFloat;
			LowPassPrev = InputFloat;
		}
	}
}

__attribute__((section("sram_func")))
float ADC_Channel::GetFloat()
{
	return InputFloat;
}

__attribute__((section("sram_func")))
int16_t ADC_Channel::GetInt()
{
	// Be careful with this function, will always return signed int even if Range is unipolar
	return input; //TODO: Error in layout!!
}

void ADC_Channel::SetLowPass(bool onoff, float Weight)
{
	LowPassOn = onoff;
	LowPassWeight = Weight;
	LowPassReset = true;
}

void ADC_Channel::ResetLowPass()
{
	LowPassReset = true;
}
