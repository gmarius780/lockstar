/*
 * SinglePIDModule.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: qo
 */

#include "SinglePIDModule.hpp"
#include "SinglePIDModuleDP.hpp"
#include <functional>
#include <cstdio>

SinglePIDModule::SinglePIDModule() : Module(){

}

SinglePIDModule::~SinglePIDModule() {
	// TODO Auto-generated destructor stub
}

void SinglePIDModule::init() {
	Module::init();
	/* Set up PID functionality */
	this->pid_loop = new PID();
	this->pid_loop->max = 10.0;//2.0;
	this->pid_loop->min = -10;//1.6;
	this->pid_loop->Kp = 0.05;
	this->pid_loop->Ki = 0.00000;
	this->pid_loop->pol = true;

	//read in pid values --> new state??

	this->oscilloscope = new Oscilloscope(this->rpi, this->adc_dev, this->dac_1, this->dac_2);
	this->oscilloscope->AddChannel(OSCILLOSCOPE_REC_ADC1);
	this->oscilloscope->AddChannel(OSCILLOSCOPE_REC_ADC2);
	this->oscilloscope->Setup(0.01, 200000.0f);

	turn_LED5_on();
}

void SinglePIDModule::work() {
	this->adc_dev->Read();
	while(!this->adc_dev->isReady());

	if(this->pid_state == pid_locked) {
		DigitalOutLow();
		this->dac_2->WriteFloat(this->pid_loop->CalculateFeedback(this->adc_dev->Channel1->GetFloat(), this->adc_dev->Channel2->GetFloat()));
		//PhilipL: dac_2->WriteFloat(PIDLoop->CalculateFeedback(PIDLoop->set_point, FFTCorr->CalculateCorrection(adc_dev->Channel2->GetFloat())));
	}
	else {
		this->dac_2->WriteFloat(0.0);//dac_2->GetMin());
		DigitalOutHigh();
	}

	this->oscilloscope->Input();

}

uint8_t SinglePIDModule::handle_rpi_input() {
	SinglePIDMethods method_identifier = (SinglePIDMethods)Module::handle_rpi_input();


	switch(method_identifier) {
	case SinglePIDMethods::INITIALIZE:
		std::apply(&SinglePIDModule::initialize, std::tuple_cat(std::make_tuple(this), SinglePIDModuleDP::read_initialize_call(this->rpi->ReadBuffer)));
		break;
	case SinglePIDMethods::SET_PID:
		break;
//		case RPi_Command_SetPIDParameters:
//		{
////			volatile uint8_t Channel  = this->rpi->ReadBuffer[1];
//			this->pid_loop->SetPIDParam((float*)(this->rpi->ReadBuffer+2));
//			//	#if defined(DOUBLE_PID) || defined(NESTED_LOCK)
//			//			PIDLoop2->SetPIDParam((float*)(RPi->ReadBuffer+2));
//			//	#endif
//			break;
//		}
//
//		case RPi_Command_StopLock:
//			this->pid_state = pid_unlocked;
//			turn_LED6_off();
//			break;
//
//		case RPi_Command_StartLock:
//			this->pid_state = pid_locked;
//			turn_LED6_on();
//			break;
//
//		case RPi_Command_MeasureAOMResponse:
//		{
//			uint8_t ADC_Channel = this->rpi->ReadBuffer[1];
//			uint8_t DAC_Channel = this->rpi->ReadBuffer[2];
//			this->dac_2->Cal->CalibrationOn = false;
//			this->dac_2->WriteFloat(0.00f);
//			while(!this->dac_2->isReady());
//			this->adc_dev->Read();
//			while(!this->adc_dev->isReady());
//			HAL_Delay(1000);
//			//float* data = RecordTrace(ADC_DEV, ADC_Channel, (DAC_Channel==1) ? DAC_1 : DAC_2, 1000);
//			float* data = RecordTrace(this->adc_dev, ADC_Channel==1 ? true : false, ADC_Channel==2 ? true : false, (DAC_Channel==1) ? this->dac_1 : this->dac_2, 0.0f, 5.0f, 1000);
//			// send
//			this->rpi->Transfer(this->rpi->ReadBuffer, (uint8_t*)data, 4000);
//			while(!this->rpi->isReady());
//			// wait until done
//			delete data;
//			break;
//		}
//
//		case RPi_Command_RecordTrace:
//		{
//			// read parameters
//			bool Channel1 = this->rpi->ReadBuffer[1];
//			bool Channel2  = this->rpi->ReadBuffer[2];
//			uint8_t DAC_Channel  = this->rpi->ReadBuffer[3];
//			float From = *((float*)(this->rpi->ReadBuffer+4));
//			float To = *((float*)(this->rpi->ReadBuffer+8));
//			uint32_t Steps = *((uint16_t*)(this->rpi->ReadBuffer+12));
//			// move to starting position
//			Move2Voltage((DAC_Channel==1) ? this->dac_1 : this->dac_2, From);
//			// turn low pass on
//			/*if (Channel1)
//				ADC_DEV->Channel1->SetLowPass(true, 0.9f);
//			if (Channel2)
//				ADC_DEV->Channel2->SetLowPass(true, 0.9f);*/
//			// measure trace
//			float* data = RecordTrace(this->adc_dev, Channel1, Channel2, (DAC_Channel==1) ? this->dac_1 : this->dac_2, From, To, Steps);
//			// turn off low pass
//			/*if (Channel1)
//				ADC_DEV->Channel1->SetLowPass(false, 0.9f);
//			if (Channel2)
//				ADC_DEV->Channel2->SetLowPass(false, 0.9f);*/
//			// send data
//			this->rpi->Write((uint8_t*)data, (Channel1 && Channel2) ? 8*Steps : 4*Steps);
//			while(!this->rpi->isReady());
//			// wait until done
//			delete data;
//			break;
//		}
//
//		case RPi_Command_ProgramCalibration:
//		{
//			// read parameters
////			volatile uint8_t DAC_Channel = RPi->ReadBuffer[1];
////			volatile float Min = *((float*)(this->rpi->ReadBuffer+2));
////			volatile float Max = *((float*)(this->rpi->ReadBuffer+6));
////			volatile uint32_t NumberPivots = *((uint32_t*)(this->rpi->ReadBuffer+10));
////
////			// transfer data
////			float* Pivots = new float[NumberPivots];
////			float* Dummy = new float[NumberPivots];
////			this->rpi->Transfer((uint8_t*)Pivots, (uint8_t*)Dummy, 4*NumberPivots);
////			while(!this->rpi->isReady());
////
////			// calibrate DAC
////			this->dac_2->Calibrate(Min, Max, Pivots, NumberPivots);
////
////			// clean up
////			delete Pivots;
////			delete Dummy;
//			break;
//		}
	}

	return (uint8_t)method_identifier;
}

void SinglePIDModule::initialize(float p, float i, float d, float out_range_min, float out_range_max, bool useTTL, bool locked,
		HardwareComponents err_channel, HardwareComponents setpoint_channel, HardwareComponents out_channel) {
	this->rpi_init();
	printf("asdf");
}
