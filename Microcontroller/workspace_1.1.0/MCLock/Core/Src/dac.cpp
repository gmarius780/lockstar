/*
 * dac.cpp
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#include "dac.hpp"
#include "newdma.hpp"
#include "stm32f4xx_hal.h"


DAC_Dev::DAC_Dev(uint8_t SPI, uint8_t DMA_Stream, uint8_t DMA_Channel, GPIO_TypeDef* SYNC_Port, uint16_t SYNC_Pin, GPIO_TypeDef* CLEAR_Port, uint16_t CLEAR_Pin)
{
	this->SYNC_Port = SYNC_Port;
	this->SYNC_Pin = SYNC_Pin;
	this->CLEAR_Port = CLEAR_Port;
	this->CLEAR_Pin = CLEAR_Pin;
	this->buffer = new uint8_t[3];
	this->ready = true;
	this->InvStepSize = 1/0.0000244140625;

	this->last_output = 0;

	this->DMAHandler = new SPI_DMA_Handler(SPI, NONE, NONE, DMA_Stream, DMA_Channel, 2);

	this->Cal = new DAC_Calibration();

	// Disable Clear-bit from start
	HAL_GPIO_WritePin(CLEAR_Port, CLEAR_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SYNC_Port, SYNC_Pin, GPIO_PIN_SET);

}

__attribute__((section("sram_func")))
void DAC_Dev::WriteInt(int32_t value)
{
	// Can only output 20-bit integers, clips the the 12 front bits and sends the value as two's complement
	// in a 24-bit message, where the prefix 0b0001 declares to write to DAC register

	// device still busy?
	if(!ready)
		return;

	// mark device as busy
	ready = false;
	// bring SYNC line low to prepare DAC
	//HAL_GPIO_WritePin(SYNC_Port, SYNC_Pin, GPIO_PIN_RESET);
	SYNC_Port->BSRR = (uint32_t)SYNC_Pin << 16U;

	if(this->invert)
		value = -value;

	// Doesn't check that value is 20bit, may get unexpected results
	buffer[0] = 0b00010000 + ((value>>16) & 0x0f);
	buffer[1] = (value>>8) & 0xff;
	buffer[2] = value & 0xff;

	// send non-blocking
	//HAL_SPI_Transmit_DMA(hspi, buffer, 3);
	//HAL_SPI_Transmit(hspi, buffer, 3,10000);
	DMAHandler->Transfer(NULL, buffer, 3);
	// when done, HAL_SPI_TxCpltCallback is run, which calls Callback()

}


void DAC_Dev::SendClearbit(int32_t value)
{
	// Sends a Clear bit that can be output by EnableClearbit()

	// device still busy?
	if(!ready)
		return;

	// mark device as busy
	ready = false;
	// bring SYNC line low to prepare DAC
	//HAL_GPIO_WritePin(SYNC_Port, SYNC_Pin, GPIO_PIN_RESET);
	SYNC_Port->BSRR = (uint32_t)SYNC_Pin << 16U;

	// Doesn't check that value is 20bit, may get unexpected results
	buffer[0] = 0b00110000 + ((value>>16) & 0x0f);
	buffer[1] = (value>>8) & 0xff;
	buffer[2] = value & 0xff;

	// send non-blocking
	//HAL_SPI_Transmit_DMA(hspi, buffer, 3);
	//HAL_SPI_Transmit(hspi, buffer, 3,10000);
	DMAHandler->Transfer(NULL, buffer, 3);

}


// Sends the configuration to the Control Register, look at Data-sheet for all available Options
void DAC_Dev::SendControlbit(float FullRange)
{

	// device still busy?
	if(!ready)
		return;

	// mark device as busy
	ready = false;
	// bring SYNC line low to prepare DAC
	SYNC_Port->BSRR = (uint32_t)SYNC_Pin << 16U;

	// depending on the output range, the DAC applies a correction to improve linear behavior
	uint8_t Lincomp = 0b0000;
	if(FullRange<10.0f)
		Lincomp = 0b0000;
	else if(FullRange<12.0f)
		Lincomp = 0b1001;
	else if(FullRange<16.0f)
		Lincomp = 0b1010;
	else if(FullRange<19.0f)
		Lincomp = 0b1011;
	else if(FullRange>=19.0f)
		Lincomp = 0b1100;

	// construct control register, see datasheet
	bool RBUF = true;
	bool OPGND = false;
	bool DACTRI = false;
	bool NOT2C = false;
	bool SDODIS = false;
	buffer[0] = 0b00100000;
	buffer[1] = Lincomp>>2;
	buffer[2] = ((Lincomp & 0b11)<<6)+(RBUF<<1)+(OPGND<<2)+(DACTRI<<3)+(NOT2C<<4)+(SDODIS<<5);

	// send configuration
	DMAHandler->Transfer(NULL, buffer, 3);

}


void DAC_Dev::EnableClearbit(bool enable)
{
	// Outputs the preprogrammed Clear-bit, has to be disabled to output with WriteInt()

	if(enable) {
		//HAL_GPIO_WritePin(CLEAR_Port, CLEAR_Pin, GPIO_PIN_RESET);
		CLEAR_Port->BSRR = (uint32_t)CLEAR_Pin << 16U; }
	else {
		//HAL_GPIO_WritePin(CLEAR_Port, CLEAR_Pin, GPIO_PIN_SET);
		CLEAR_Port->BSRR = (uint32_t)CLEAR_Pin; }
}

__attribute__((section("sram_func")))
void DAC_Dev::Callback()
{
	DMAHandler->Callback();
	// wait for TXE flag going to 1 and busy flag going to 0
	//while((!( this->hspi->Instance->SR & SPI_SR_TXE )) & ( this->hspi->Instance->SR & SPI_SR_BSY ))
	//	;
	// bring SYNC line up to finish DA conversion
	// (The DA conversion is completed automatically with the 24th transmitted bit. The SYNC line has to go high at least 20ish ns before the next data package, so it
	// could also be done at a later point, if more convenient / faster.)
	//HAL_GPIO_WritePin(SYNC_Port, SYNC_Pin, GPIO_PIN_SET);
	SYNC_Port->BSRR = (uint32_t)SYNC_Pin;
	// mark device as ready
	ready = true;
}

__attribute__((section("sram_func")))
void DAC_Dev::WriteFloat(float value)
{
	// apply limits
	float output = value;
	if(output<LimitLow)
		output = LimitLow;
	if(output>LimitHigh)
		output = LimitHigh;
	// save output
	last_output = output;
	// write output to device
	if(Cal->CalibrationOn)
		WriteInt((int32_t)((Cal->ApplyCalibration(value)-ZeroVoltage) * InvStepSize - 0.5f));
	else
		WriteInt((int32_t)((value-ZeroVoltage) * InvStepSize - 0.5f)); // -0.5 because of Two's complement and rounding towards 0
}

// set up the DAC device for specific output range
void DAC_Dev::Setup(uint8_t output_range, bool invert)
{
	config = output_range;

	float UpperVoltage, LowerVoltage;
	switch(output_range){
	case DAC_UNIPOLAR_5V:
		UpperVoltage = 5.0f;
		LowerVoltage = 0.0f;
		break;
	case DAC_UNIPOLAR_10V:
		UpperVoltage = 10.0f;
		LowerVoltage = 0.0f;
		break;
	case DAC_BIPOLAR_5V:
		UpperVoltage = 5.0f;
		LowerVoltage = -5.0f;
		break;
	case DAC_BIPOLAR_10V:
		UpperVoltage = 10.0f;
		LowerVoltage = -10.0f;
	}

	float FullRange = UpperVoltage - LowerVoltage;
	this->ZeroVoltage = (UpperVoltage+LowerVoltage)/2.0f;
	this->StepSize = FullRange / 0xfffff;	// FullRange / (2^20-1)
	this->InvStepSize = 1 / StepSize;
	this->invert=invert;

	this->SendControlbit(FullRange);
}

void DAC_Dev::Calibrate(float Min, float Max, float* Pivots, uint32_t NumberPivots)
{
	// use calibration pivots for limiting the DAC output
	this->LimitLow = Min;//Pivots[0];
	this->LimitHigh= Max;//Pivots[NumberPivots-1];
	// save calibration data in calibration object
	this->Cal->Calibrate(Min, Max, Pivots, NumberPivots);
}

DAC_Calibration::DAC_Calibration()
{
	CalibrationOn = false;
}

void DAC_Calibration::Calibrate(float Min, float Max, float* Pivots, uint32_t NumberPivots)
{
	this->Min = Min;
	this->NumberPivots = NumberPivots;
	OneOverStep = (NumberPivots-1) / (Max-Min);
	// delete previous calibration
	if(this->Pivots==NULL)
		delete this->Pivots;
	// copy pivots
	this->Pivots = new float[NumberPivots];
	for(uint32_t i=0; i<NumberPivots; i++)
		this->Pivots[i] = Pivots[i];
	// activate calibrations
	CalibrationOn = true;
}


// apply calibration by linearly interpolating between two pivots
__attribute__((section("sram_func")))
float DAC_Calibration::ApplyCalibration(float TargetOutput)
{
	float FloatIndex = (TargetOutput-Min) * OneOverStep;
	uint32_t IntIndex = (uint32_t)FloatIndex;
	return ((1+IntIndex-FloatIndex)*Pivots[IntIndex]+(FloatIndex-IntIndex)*Pivots[IntIndex+1]);
}
