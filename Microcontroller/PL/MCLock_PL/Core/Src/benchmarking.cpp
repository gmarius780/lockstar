/*
 * benchmarking.cpp
 *
 *  Created on: Mar 3, 2020
 *      Author: qo
 */

/*
#include "benchmarking.hpp"
#include "spi.h"

AnalogBenchmarker::AnalogBenchmarker(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, uint32_t Samples, SPI_HandleTypeDef* RPi)
{
	this->ADC_DEV = ADC_DEV;
	this->DAC_1 = DAC_1;
	this->DAC_2 = DAC_2;
	this->Samples = Samples;
	this->RPi = RPi;
	this->Data = new float[Samples];
}

void AnalogBenchmarker::RecordData(uint32_t channel)
{
	for(uint32_t i = 0; i<Samples; i++) {
		ADC_DEV->Read();
		while(!ADC_DEV->isReady());
		Data[i] = (channel==1) ? ADC_DEV->Channel1->GetFloat() : ADC_DEV->Channel2->GetFloat();
	}
}

void AnalogBenchmarker::SendData()
{
	Pi_WriteFloatChunk(RPi, Data, Samples);
}


void RunAnalogBenchmark(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, uint32_t Samples, SPI_HandleTypeDef* RPi)
{
	AnalogBenchmarker AB = AnalogBenchmarker(ADC_DEV, DAC_1, DAC_2, Samples, RPi);

	for(uint8_t i=0; i<4; i++) {
		volatile uint16_t test = Pi_ReadUInt16(RPi);
		AB.RecordData(i%2 + 1);
		AB.SendData();
	}
}

*/
