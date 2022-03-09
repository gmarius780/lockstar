#include "lock.hpp"
#include "stm32f4xx_hal_tim.h"
#include <vector>
#include <math.h>
#include <main.h>

#define max(x,y) ((x<y)?y:x)

void lockparameters::AddWaypoint(float wp)
{
	this->waypoints.push_back(wp);
	this->prevwaypoints.push_back(wp);
	this->empty = false;
}

void lockparameters::AddWaypoints(float* wps, uint16_t size)
{
	for(int i=0;i<size;i++){
		this->waypoints.push_back(wps[i]);
		this->prevwaypoints.push_back(wps[i]);
	}
	this->empty = false;
}


bool lockparameters::CheckLock()
{
	float variancetol = 0.5;

	bool res = false;
	if(this->prevprevvariance==0)
		res = true;
	if(abs(this->variance-this->prevprevvariance)<(max(this->variance,this->prevprevvariance)*(variancetol)))
		res = true;
	this->prevprevvariance = this->prevvariance;
	this->prevvariance = this->variance;
	this->variance = 0;
	return res;
}

void lockparameters::RestartLock()
{
	this->currentvalue = _20BIT_MIN; //this->startingpoint;
	this->waypoints = this->prevwaypoints;
	this->empty = false;
	this->locked = false;
	this->damping = true;
	this->cmax = this->prevcmax;
	this->integrator = 0;
	this->variance = 0;
	this->prevvariance = 0;
	this->prevprevvariance = 0;
	this->DAC_Fast->WriteFloat(0.0);
	this->counter = 0;
	this->pidp->Reset();
	this->reset = true;
	HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin, GPIO_PIN_RESET);
}

float Damping(uint32_t counter){
	// TODO: Check if damping is still necessary and automate finding coeficients
	float a = -0.162f;
	float b = -0.00028f;
	float c = 0.162f;

	return a*exp(counter*b)+c;
}

float lockparameters::LockStep(){
	float peaktol = 0.3;
	float Ki = 0.1;
	uint32_t Check_Lock_Period = 4000; // Amount of sampling ticks
	uint32_t Damping_Duration = 5000;

	float output = 0;
	float err = this->ADC_Error->GetFloat();
	if(this->reset){
		this->currentvalue++;
		this->DAC_Slow->WriteInt(this->currentvalue);
		if(this->currentvalue>=this->startingpoint)
			this->reset = false;
	}else if(this->locked){
		output = this->pidp->CalculateFeedback(this->offset,err);
		this->DAC_Fast->WriteFloat(output);
		(this->damping) ? (this->integrator= Damping(this->counter)) : (this->integrator+=output*Ki);
		this->DAC_Slow->WriteInt(this->DAC_Slow->Float2Int(this->DAC_Slow->Int2Float(this->currentvalue)+this->integrator));
		this->variance += (err-this->offset)*(err-this->offset);
		this->counter++;
		if(this->damping && (this->counter>=Damping_Duration)){
			this->damping = false;
			this->counter = 0;
			this->variance = 0;
		}
		if(!(this->damping) && (this->counter>=Check_Lock_Period)){
			if(!this->CheckLock())
				this->RestartLock();
			this->counter = 0;
		}

	} else if(this->empty){
		this->DAC_Slow->WriteInt(this->currentvalue);
		if((this->cmax) ? (err>this->offset) : (err<this->offset)){
			this->locked = true;
			HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin, GPIO_PIN_SET);
		}
		else {
			this->Step();
			this->DAC_Slow->WriteInt(this->currentvalue);
		}
	} else {
		float cwp = this->waypoints.front();
		this->Step();
		if(this->cmax ? ((err-this->offset)>(cwp-this->offset)*(1-peaktol)) : ((err-this->offset)<(cwp-this->offset)*(1-peaktol))){
			int wps_left = this->CheckWaypoint();
			if(!wps_left){
				this->empty = true;
			}
		}
		else {
			this->DAC_Slow->WriteInt(this->currentvalue);
			if(this->currentvalue>=_20BIT_MAX)
				this->RestartLock();
		}
	}
	return this->currentvalue;
}
