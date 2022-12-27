/*
 * lock.hpp
 *
 *  Created on: Mar 11, 2016
 *      Author: philip
 */

//#include "stm32f4xx_hal_tim.h"

#ifndef LOCK_H_
#define LOCK_H_

#include <vector>


#include "../Lib/pid.hpp"

#ifdef OLD
#include "../HAL/dac.hpp"
#include "../HAL/adc.hpp"
class lockparameters
{
private:
	int32_t startingpoint;
	int32_t currentvalue;
	uint32_t counter = 0;
	bool direction;
	bool cmax;
	bool prevcmax;
	bool empty = true;
	bool locked = false;
	bool damping = true;
	bool reset = false;
	bool debug = false;
	float offset;
	float transmissionoffset;
	float integrator = 0;
	float variance = 0;
	float prevvariance = 0;
	float prevprevvariance = 0;
	TIM_HandleTypeDef *htim;
	ADC_Channel *ADC_Transm, *ADC_Error;
	DAC_Dev *DAC_Slow, *DAC_Fast;
public:
	std::vector<float> waypoints;
	std::vector<float> prevwaypoints;
	PID* pidp;
	void AddWaypoint(float wp);
	void AddWaypoints(float* wps, uint16_t size);
	void SetPolarity(bool polarity) { pidp->pol = polarity; };
	void SetPidVars(float* Vars) { pidp->SetVars(Vars);};
	void SetStartingPoint(int32_t startingpoint) { this->startingpoint = startingpoint; this->currentvalue = startingpoint; };
	void SetDirection(bool direction) { this->direction = direction; };
	void SetOffset(float offset) { this->offset = offset;};
	void SetTransmissionOffset(float offset) { this->transmissionoffset = offset;};
	void SetCurrentMax(bool max) { this->cmax = max; this->prevcmax = max;};
	void SetDebug(bool debug) { this->debug = debug; };
	bool Debug() { return this->debug; };
	float CurrentWaypoint() { return this->waypoints.front(); };
	int CheckWaypoint() { this->cmax = !(this->cmax); this->waypoints.erase(this->waypoints.begin()); return this->waypoints.size(); };
	int32_t Step() { return (direction ? (++currentvalue) : (--currentvalue)); };
	void Lock();
	float LockStep();
	bool CheckLock();
	void RestartLock();
	lockparameters(PID* pidp, TIM_HandleTypeDef *htim, ADC_Channel* ADC_Transm, ADC_Channel* ADC_Error, DAC_Dev* DAC_Slow, DAC_Dev* DAC_Fast) { this->pidp = pidp; this->htim = htim; this->ADC_Transm = ADC_Transm; this->ADC_Error = ADC_Error; this->DAC_Slow = DAC_Slow; this->DAC_Fast = DAC_Fast; }
};

#endif /* LOCK_H_ */
#endif
