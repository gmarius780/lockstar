/*
 * SinglePIDModule.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: qo
 */

#include "SinglePIDModule.hpp"

SinglePIDModule::SinglePIDModule() {
	// TODO Auto-generated constructor stub

}

SinglePIDModule::~SinglePIDModule() {
	// TODO Auto-generated destructor stub
}

void SinglePIDModule::init() {
	Module::init();
	/* Set up PID functionality */
	this->PIDLoop = new PID();
	this->PIDLoop->max = 10.0;//2.0;
	this->PIDLoop->min = -10;//1.6;
	this->PIDLoop->Kp = 0.05;
	this->PIDLoop->Ki = 0.00000;
	this->PIDLoop->pol = true;

	//read in pid values --> new state??
}

void SinglePIDModule::work() {
	this->ADC_DEV->Read();
	while(!this->ADC_DEV->isReady());

	/*if(locking) {
		DigitalOutLow();
		this->DAC_2->WriteFloat(this->PIDLoop->CalculateFeedback(this->ADC_DEV->Channel1->GetFloat(), this->ADC_DEV->Channel2->GetFloat()));
		//PhilipL: DAC_2->WriteFloat(PIDLoop->CalculateFeedback(PIDLoop->set_point, FFTCorr->CalculateCorrection(ADC_DEV->Channel2->GetFloat())));
	}
	else {
		this->DAC_2->WriteFloat(0.0);//DAC_2->GetMin());
		DigitalOutHigh();
	}*/

}
