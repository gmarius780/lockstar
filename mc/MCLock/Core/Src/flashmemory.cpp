/*
 * flashmemory.cpp
 *
 *  Created on: 13 May 2020
 *      Author: qo
 */

#include "flashmemory.hpp"
#include "stm32f4xx_hal.h"
#include "adc.hpp"
#include "dac.hpp"
#include "oscilloscope.hpp"
#include "pid.hpp"

UserData::UserData(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, PID* PID_1, PID* PID_2, Oscilloscope* Scope)
{
	flash_address = 0x081E0000UL;
	this->ADC_DEV = ADC_DEV;
	this->DAC_1 = DAC_1;
	this->DAC_2 = DAC_2;
	this->PID_1 = PID_1;
	this->PID_2 = PID_2;
	this->Scope = Scope;
}

// This function saves a data array into the non-volatile flash memory of the microcontroller. The data can be loaded again after a reset.
void UserData::WriteFlashPage(uint32_t* data, uint32_t length)
{
	// erase page
	uint32_t PAGEError = 0;
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = FLASH_SECTOR_23;
	EraseInitStruct.NbSectors = 1;
	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
	// write data into page
	for(uint32_t i=0; i<length; i++)
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address+4*i, *(data+i));
	HAL_FLASH_Lock();
}


/* saved parameters
 * - ADC 1 & 2: config, LowPassOn, LowPassWeight
 * - DAC 1 & 2: config, invert, LimitLow, LimitHigh, Calibration (On, Min, OneOverStep, NumberPivots, Pivots)
 * - PID 1 & 2: kp, ki, kd, pol, min, max
 * - Scope: skips, delay
 */

// The following two functions coordinate saving parameters into the flash by assigning memory positions to each relevant variable
void UserData::LoadSettings(uint32_t* SavedData)
{
	uint32_t index=0;
	// ADC Channel 1
	ADC_DEV->Channel1->Setup((uint8_t)SavedData[index++]);
	ADC_DEV->Channel1->LowPassOn = (uint8_t)SavedData[index++];
	ADC_DEV->Channel1->LowPassWeight = *((float*)(SavedData+index++));
	// ADC Channel 2
	ADC_DEV->Channel2->LowPassOn = (uint8_t)SavedData[index++];
	ADC_DEV->Channel2->LowPassWeight = *((float*)(SavedData+index++));
	ADC_DEV->Channel2->Setup((uint8_t)SavedData[index++]);
	// DAC Channel 1


}

void UserData::SaveSettings()
{
	uint32_t index=0;
	uint32_t SaveData[30];
	// ADC Channel 1
	SaveData[index++] = (uint32_t)(ADC_DEV->Channel1->config);
	SaveData[index++] = (uint32_t)(ADC_DEV->Channel1->LowPassOn);
	*((float*)(SaveData+index++)) = (ADC_DEV->Channel1->LowPassWeight);
	// ADC Channel 2
	SaveData[index++] = (uint32_t)(ADC_DEV->Channel1->config);
	SaveData[index++] = (uint32_t)(ADC_DEV->Channel1->LowPassOn);
	*((float*)(SaveData+index++)) = (ADC_DEV->Channel1->LowPassWeight);
	// DAC Channel 1
	SaveData[index++] = (uint32_t)(DAC_1->config);
	SaveData[index++] = (uint32_t)(DAC_1->invert);
	*((float*)(SaveData+index++)) = (DAC_1->LimitLow);
	*((float*)(SaveData+index++)) = (DAC_1->LimitHigh);
	// DAC Channel 1 Calibration
	*((float*)(SaveData+index++)) = (DAC_1->Cal->CalibrationOn);
	*((float*)(SaveData+index++)) = (DAC_1->Cal->Min);
	*((float*)(SaveData+index++)) = (DAC_1->Cal->OneOverStep);
	SaveData[index++] = (uint32_t)(DAC_1->Cal->NumberPivots);
	for(uint32_t i=0; i<DAC_1->Cal->NumberPivots; i++)
		*((float*)(SaveData+index++)) = (DAC_1->Cal->Pivots[i]);
	// DAC Channel 2
	SaveData[index++] = (uint32_t)(DAC_2->config);
	SaveData[index++] = (uint32_t)(DAC_2->invert);
	*((float*)(SaveData+index++)) = (DAC_2->LimitLow);
	*((float*)(SaveData+index++)) = (DAC_2->LimitHigh);
	// DAC Channel 2 Calibration
	*((float*)(SaveData+index++)) = (DAC_2->Cal->CalibrationOn);
	*((float*)(SaveData+index++)) = (DAC_2->Cal->Min);
	*((float*)(SaveData+index++)) = (DAC_2->Cal->OneOverStep);
	SaveData[index++] = (uint32_t)(DAC_2->Cal->NumberPivots);
	for(uint32_t i=0; i<DAC_2->Cal->NumberPivots; i++)
		*((float*)(SaveData+index++)) = (DAC_2->Cal->Pivots[i]);
}
