/*
 * oscilloscope.hpp
 *
 *  Created on: Mar 6, 2020
 *      Author: qo
 */

#ifndef INC_OSCILLOSCOPE_HPP_
#define INC_OSCILLOSCOPE_HPP_

//#include "raspberrypi.hpp"
//#include "adc.hpp"
//#include "dac.hpp"
#include <vector>
class RaspberryPi;
class ADC_Dev;
class DAC_Dev;

#define OSCILLOSCOPE_POINTS 1000UL
#define OSCILLOSCOPE_UPDATE_RATE 1.0f
#define OSCILLOSCOPE_REC_ADC1 1UL
#define OSCILLOSCOPE_REC_ADC2 2UL
#define OSCILLOSCOPE_REC_DAC1 3UL
#define OSCILLOSCOPE_REC_DAC2 4UL

class Oscilloscope
{
private:
	uint8_t* DataBuffer;
	volatile uint16_t skips;
	volatile uint32_t delay;
	RaspberryPi* RPi;
	ADC_Dev* ADC_DEV;
	DAC_Dev* DAC_1, *DAC_2;
	volatile uint32_t buffer_index, skip_counter;
	volatile uint32_t counter = 0;
	volatile uint32_t delay_counter = 0;
	uint32_t ArrayLimit;
	std::vector<volatile float*> Channels;
public:
	Oscilloscope(RaspberryPi* RPi, ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2);
	void AddChannel(uint32_t Channel);
	void Input();
	void InputBurst();
	void Setup(float TimeWindow, float SamplingFrequency);
	void Setup(uint16_t skips);
	void Reset();
	void Input(float value);
	void Input(float value1, float value2);
	void modify(float value){ *(((float*)DataBuffer) + 100) = value; };

	friend class UserData;
};


#endif /* INC_OSCILLOSCOPE_HPP_ */
