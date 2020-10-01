/*
 * dac.h
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

#ifndef DAC_H_
#define DAC_H_

#include "stm32f4xx_hal.h"
#include "newdma.hpp"
#include "oscilloscope.hpp"

#define DAC_BIPOLAR_10V		(uint8_t)0
#define DAC_BIPOLAR_5V		(uint8_t)1
#define DAC_UNIPOLAR_10V	(uint8_t)2
#define DAC_UNIPOLAR_5V		(uint8_t)3

class DAC_Calibration
{
private:
	float Min, OneOverStep;
	float* Pivots;
	uint32_t NumberPivots;
public:
	DAC_Calibration();
	void Calibrate(float Min, float Max, float* Pivots, uint32_t NumberPivots);
	bool CalibrationOn;
	float ApplyCalibration(float TargetOutput);

	friend class UserData;
};

class DAC_Dev
{
private:

	// hardware definitions
	GPIO_TypeDef* SYNC_Port;
	uint16_t SYNC_Pin;
	GPIO_TypeDef* CLEAR_Port;
	uint16_t CLEAR_Pin;

	// internal operations
	volatile bool ready;
	SPI_DMA_Handler* DMAHandler;
	bool invert;
	volatile float last_output;
	uint8_t *buffer;

	// int to float conversion
	uint8_t config;
	uint8_t control_reg;
	float InvStepSize;
	float StepSize;
	float ZeroVoltage;

	// DMA config
	volatile uint8_t* DMAConfig;

	/*float* Aom_coeff;
	uint16_t* lin_aom;
	bool lin_aom_ready;
	float* LinearizationPivots;
	float LinearizationStep, LinearizationOneOverStep;
	uint32_t Linearize(float TargetOutput);*/

	// output limitation
	float LimitHigh, LimitLow;

public:

	// constructor
	DAC_Dev(uint8_t SPI, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, GPIO_TypeDef* SYNC_Port, uint16_t SYNC_Pin, GPIO_TypeDef* CLEAR_Port, uint16_t CLEAR_Pin);

	// output functions
	void WriteFloat(float value);
	void WriteInt(int32_t value);

	// callback function to clean up after writing
	void Callback();

	// all operations done?
	bool isReady() { return ready; };

	// int to float conversion
	float Int2Float(int32_t input) { return (input*StepSize)+ZeroVoltage; };
	int32_t Float2Int(float input) { return (int32_t)((input-ZeroVoltage) * InvStepSize + 0.5); };

	// setup
	void SetLimits(float min, float max){ this->LimitLow=min; this->LimitHigh=max; };
	void Setup(uint8_t output_range, bool invert);

	void SendClearbit(int32_t value);
	void SendControlbit(float FullRange);
	void EnableClearbit(bool enable);

	void MakeDMA2Way();
	void MakeDMA1Way();

	// getter functions
	float GetLast(){ return this->last_output; };
	float GetMax(){ return LimitHigh; };
	float GetMin(){ return LimitLow; };
	uint8_t GetControlReg(){ return control_reg; };

	// self test
	void ReadControlReg_Step1();
	uint8_t* ReadControlReg_Step2();

	// output calibration
	DAC_Calibration* Cal;
	void Calibrate(float Min, float Max, float* Pivots, uint32_t NumberPivots);

	// these classes have access to the private elements
	friend class Oscilloscope;
	friend class UserData;

};

#endif /* DAC_H_ */
