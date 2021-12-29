/*
 * benchmarking.hpp
 *
 *  Created on: Mar 3, 2020
 *      Author: qo
 */

#ifndef INC_BENCHMARKING_HPP_
#define INC_BENCHMARKING_HPP_

#include "adc.hpp"
#include "dac.hpp"

class AnalogBenchmarker
{
private:
	uint32_t Samples;
	float* Data;
	ADC_Dev* ADC_DEV;
	DAC_Dev *DAC_1, *DAC_2;
	SPI_HandleTypeDef* RPi;
public:
	AnalogBenchmarker(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, uint32_t Samples, SPI_HandleTypeDef* RPi);
	void RecordData(uint32_t channel);
	void SendData();
};

void RunAnalogBenchmark(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, uint32_t Samples, SPI_HandleTypeDef* RPi);


#endif /* INC_BENCHMARKING_HPP_ */
