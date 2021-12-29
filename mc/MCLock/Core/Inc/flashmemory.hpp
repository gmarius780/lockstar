/*
 * flashmemory.hpp
 *
 *  Created on: 13 May 2020
 *      Author: qo
 */

#ifndef INC_FLASHMEMORY_HPP_
#define INC_FLASHMEMORY_HPP_

#include "stm32f4xx_hal.h"
//#include "adc.hpp"
//#include "dac.hpp"
//#include "oscilloscope.hpp"
//#include "pid.hpp"
class ADC_Dev;
class DAC_Dev;
class PID;
class Oscilloscope;

class UserData
{
private:
	void WriteFlashPage(uint32_t* data, uint32_t length);
	ADC_Dev* ADC_DEV;
	DAC_Dev *DAC_1, *DAC_2;
	PID *PID_1, *PID_2;
	Oscilloscope* Scope;
	uint32_t flash_address;
public:
	UserData(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, PID* PID_1, PID* PID_2, Oscilloscope* Scope);
	void LoadSettings(uint32_t* SavedData);
	void SaveSettings();

};

#endif /* INC_FLASHMEMORY_HPP_ */
