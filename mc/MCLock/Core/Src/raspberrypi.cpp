/*
 * raspberrypi.cpp
 *
 *  Created on: 4 Mar 2020
 *      Author: qo
 */


#include "raspberrypi.hpp"

RaspberryPi::RaspberryPi(uint8_t SPI, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, GPIO_TypeDef* Int_Port, uint16_t Int_Pin)
{
	this->Int_Port = Int_Port;
	this->Int_Pin = Int_Pin;
	DMAHandler = new SPI_DMA_Handler(SPI, DMA_Stream_In, DMA_Channel_In, DMA_Stream_Out, DMA_Channel_Out, 1);
	this->ready = true;
	this->ReadBuffer = new uint8_t[4020];
}

__attribute__((section("sram_func")))
void RaspberryPi::Transfer(uint8_t* ReadBuffer, uint8_t* WriteBuffer, uint32_t BufferSize)
{
	if (!ready)
		return;
	ready = false;

	if(ReadBuffer == this->ReadBuffer && BufferSize>4020) {
		delete this->ReadBuffer;
		this->ReadBuffer = new uint8_t[BufferSize];
		ReadBuffer = this->ReadBuffer;
	}

	DMAHandler->Transfer(ReadBuffer, WriteBuffer, BufferSize);

	this->SetIntPin();
}

__attribute__((section("sram_func")))
void RaspberryPi::Write(uint8_t* WriteBuffer, uint32_t BufferSize)
{
	if (!ready)
		return;
	ready = false;

	DMAHandler->Transfer(NULL, WriteBuffer, BufferSize);

	this->SetIntPin();
}

__attribute__((section("sram_func")))
void RaspberryPi::Read(uint8_t* ReadBuffer, uint32_t BufferSize)
{
	if (!ready)
		return;
	ready = false;

	DMAHandler->Transfer(ReadBuffer, NULL, BufferSize);

	this->SetIntPin();
}

__attribute__((section("sram_func")))
void RaspberryPi::Callback()
{
	DMAHandler->Callback();
	//delete this->ReadBuffer;
	//this->ReadBuffer = new uint8_t[4020];
	ready = true;
}

__attribute__((section("sram_func")))
void RaspberryPi::SetIntPin()
{
	Int_Port->BSRR = Int_Pin;
}

__attribute__((section("sram_func")))
void RaspberryPi::ResetIntPin()
{
	Int_Port->BSRR = ((uint32_t)Int_Pin)<<16;
}
