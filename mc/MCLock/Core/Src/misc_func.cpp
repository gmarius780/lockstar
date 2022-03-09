/*
 * misc_func.cpp
 *
 *  Created on: 10 Mar 2020
 *      Author: qo
 */



#include "misc_func.hpp"
#include <main.h>
#include <math.h>
#include "../HAL/leds.hpp"

float* RecordTrace(ADC_Dev* ADC_DEV, uint8_t ADC_Channel, DAC_Dev* DAC_DEV, uint16_t Steps)
{
	uint32_t Range = 1048576UL; // 2 ^ 20
	uint32_t HalfRange = 524288UL; // 2 ^ 19
	uint32_t StepSize = (Range-1) / Steps;

	// data storage array
	float* data = new float[Steps];

	uint32_t output = HalfRange; // we are in two's complement, the lowest output corresponds to mid-way
	volatile uint32_t CountDownTimer;
	for(uint16_t index=0; index<Steps; index++)
	{
		// generate output
		DAC_DEV->WriteInt(output);
		while(!DAC_DEV->isReady());
		// wait briefly for output to stabilize
		CountDownTimer = 5000;
		while(CountDownTimer>0)
			CountDownTimer--;
		// read on input
		ADC_DEV->Read();
		while(!DAC_DEV->isReady());
		// save in array
		if (ADC_Channel==1)
			data[index] = ADC_DEV->Channel1->GetFloat();
		else
			data[index] = ADC_DEV->Channel2->GetFloat();
		// change target output
		output += StepSize;
		if (output>Range)
			output-=Range;
		CountDownTimer = 50000;
		while(CountDownTimer>0)
			CountDownTimer--;

	}

	return data; // make sure to delete the data array later!
}

float* RecordTrace(ADC_Dev* ADC_DEV, bool ReadChannel1, bool ReadChannel2, DAC_Dev* DAC_DEV, float From, float To, uint32_t Steps)
{
	float StepSize = (To - From) / (Steps-1);
	uint32_t BufferSize = (ReadChannel1 && ReadChannel2) ? 2*Steps : Steps;

	// data storage array
	float* data = new float[BufferSize];

	float output = From;
	volatile uint32_t CountDownTimer;
	for(uint32_t index=0; index<Steps; index++)
	{
		// generate output
		DAC_DEV->WriteFloat(output);
		while(!DAC_DEV->isReady());
		// wait briefly for output to stabilize
		CountDownTimer = 5000;
		while(CountDownTimer>0)
			CountDownTimer--;
		// read on input
		ADC_DEV->Read();
		while(!DAC_DEV->isReady());
		// save in array
		if (ReadChannel1 && ReadChannel2)
		{
			data[2*index] = ADC_DEV->Channel1->GetFloat();
			data[2*index+1] = ADC_DEV->Channel2->GetFloat();
		}
		else
			data[index] = ReadChannel1 ? ADC_DEV->Channel1->GetFloat() : ADC_DEV->Channel2->GetFloat();
		// change target output
		output += StepSize;
	}

	return data; // make sure to delete the data array later!
}

void Move2Voltage(DAC_Dev* DAC_DEV, float Voltage, float MaxStep)
{
	volatile float Start = DAC_DEV->GetLast();
	uint32_t NumberSteps = (uint32_t)(((Voltage>Start) ? (Voltage-Start) : (Start-Voltage))/MaxStep+1);
	float Step = (Voltage-Start)/(NumberSteps);

	volatile uint32_t CountDownTimer;
	for(uint32_t index=1; index<=NumberSteps; index++)
	{
		// generate output
		DAC_DEV->WriteFloat(Start+index*Step);
		while(!DAC_DEV->isReady());
		// wait briefly for output to stabilize
		CountDownTimer = 100;
		while(CountDownTimer>0)
			CountDownTimer--;
	}
	CountDownTimer = 10000;
	while(CountDownTimer>0)
		CountDownTimer--;
}

void Go2Lock(DAC_Dev* DAC_DEV, ADC_Dev* ADC_DEV, std::queue<Waypoint> waypoints, float Step)
{
	float DirectedStep = 0;
	volatile float output = DAC_DEV->GetLast();
	Waypoint* NextWaypoint;
	while (!waypoints.empty())
	{
		NextWaypoint = &waypoints.front();
		if(NextWaypoint->type==WAYPOINT_GO_TO) {
			Move2Voltage(DAC_DEV, NextWaypoint->value, Step);
			output = NextWaypoint->value;
			waypoints.pop();
			continue; }
		if(NextWaypoint->type==WAYPOINT_GO_UP) {
			DirectedStep = + Step;
			waypoints.pop();
			continue;
		}
		if(NextWaypoint->type==WAYPOINT_GO_DOWN) {
			DirectedStep = - Step;
			waypoints.pop();
			continue;
		}
		// change output
		output += DirectedStep;
		if(output<DAC_DEV->GetMin() or output>DAC_DEV->GetMax())
			break;
		DAC_DEV->WriteFloat(output);
		while(!DAC_DEV->isReady());
		// wait for input
		ADC_DEV->Read();
		while(!DAC_DEV->isReady());
		// tick off waypoints as they pass
		volatile float minmax = waypoints.front().type;
		volatile float value = waypoints.front().value;
		volatile float input = (minmax==WAYPOINT_ERRORS_LARGER || minmax==WAYPOINT_ERRORS_SMALLER) ? ADC_DEV->Channel2->GetFloat() : ADC_DEV->Channel1->GetFloat();
		if(minmax*input>minmax*value)
			waypoints.pop();
	}
}


__attribute__((section("sram_func")))
void DigitalOutHigh()
{
	DigitalOut_GPIO_Port->BSRR = DigitalOut_Pin << 16U;
}

__attribute__((section("sram_func")))
void DigitalOutLow()
{
	DigitalOut_GPIO_Port->BSRR = DigitalOut_Pin;
}

void RecordDetailedTrace(ADC_Dev* ADC_DEV, bool ReadChannel1, bool ReadChannel2, DAC_Dev* DAC_DEV, RaspberryPi* RPi, uint32_t From, uint32_t NumberSteps, uint32_t BlockSize)
{
	uint32_t BufferSize = (ReadChannel1 && ReadChannel2) ? 4*BlockSize : 2*BlockSize;
	uint32_t Range = 1048576UL; // 2 ^ 20

	// data storage array
	float* data = new float[BufferSize];

	uint32_t Output = From;
	volatile uint32_t ArrayIndex = 0;
	volatile uint32_t CountDownTimer;

	HAL_Delay(3000);

	for(uint32_t Counter = 0; Counter<NumberSteps; Counter++)
	{
		// generate output
		DAC_DEV->WriteInt(Output);
		while(!DAC_DEV->isReady());
		// read on input
		ADC_DEV->Read();
		while(!DAC_DEV->isReady());
		// save in array
		if (ReadChannel1 && ReadChannel2)
		{
			data[ArrayIndex] = ADC_DEV->Channel1->GetFloat();
			data[ArrayIndex+1] = ADC_DEV->Channel2->GetFloat();
			ArrayIndex+=2;
		}
		else
		{
			data[ArrayIndex] = ReadChannel1 ? ADC_DEV->Channel1->GetFloat() : ADC_DEV->Channel2->GetFloat();
			ArrayIndex++;
		}
		// increment target output, take care of two's complement
		Output++;
		if(Output==Range)
			Output = 0;
		// start transmission when array is half-filled
		if(ArrayIndex==BufferSize/2) {
			while(!RPi->isReady());
			RPi->Write((uint8_t*)data, 2*BufferSize);
		}
		if(ArrayIndex==BufferSize) {
			while(!RPi->isReady());
			RPi->Write((uint8_t*)(data+BufferSize/2), 2*BufferSize);
			ArrayIndex = 0;
		}
		CountDownTimer = 50;//10
		while(CountDownTimer>0)
			CountDownTimer--;

	}
	while(!RPi->isReady());
}

void TestRCCompensationJump(DAC_Dev* DAC_DEV)
{
	volatile float output = -7.0;
	turn_LED6_on();
	DAC_DEV->WriteFloat(output);
	while(!DAC_DEV->isReady());
	HAL_Delay(5000);
	DigitalOutHigh();
	uint32_t counter = 0;
	float output_increment = 0.00003;
	float RCJump = output_increment / 0.0000021 * 0.038;
	while(output<0)
	{
		output+=output_increment;
		DAC_DEV->WriteFloat(output);
		while(!DAC_DEV->isReady());
		counter++;
	}
	DigitalOutLow();
	//DAC_DEV->WriteFloat(-RCJump);
	while(!DAC_DEV->isReady());
}

void OptimizePID(ADC_Dev* ADC_DEV, uint8_t ADC_Channel, DAC_Dev* DAC_DEV, RaspberryPi* RPi, PID* PIDLoop)
{
	uint8_t cmd_buffer[4013];
	volatile uint8_t cmd = 0;
	// save pid settings
	float old_kp = PIDLoop->Kp;
	float old_ki = PIDLoop->Ki;
	float old_kd = PIDLoop->Kd;
	// set up parameters
	uint32_t counter = 0;
	float input;
	float trace[1000];
	volatile float setpoint;
	uint8_t Dummy[4013];
	while (true)
	{
		DAC_DEV->WriteFloat(DAC_DEV->GetMin());
		// read command from RPi
		RPi->Transfer(cmd_buffer, Dummy, 4013);
		while(!RPi->isReady());
		//turn_LED5_off();
		cmd = cmd_buffer[0];
		PIDLoop->Reset();

		switch(cmd) {
		case PID_ACCEPT_SETTINGS:
			PIDLoop->Kp = *((float*)(cmd_buffer+1));
			PIDLoop->Ki = *((float*)(cmd_buffer+5));
			PIDLoop->Kd = *((float*)(cmd_buffer+9));
			return;
		case PID_TRY_SETTINGS:
			PIDLoop->Kp = *((float*)(cmd_buffer+1));
			PIDLoop->Ki = *((float*)(cmd_buffer+5));
			PIDLoop->Kd = *((float*)(cmd_buffer+9));
			break;
		case PID_ABORT:
			PIDLoop->Kp = old_kp;
			PIDLoop->Ki = old_ki;
			PIDLoop->Kd = old_kd;
			return;}


		turn_LED6_on();
		// run PID for a while
		for(counter=0; counter<1000; counter++)
		{
			// read
			ADC_DEV->Read();
			while(!ADC_DEV->isReady());
			input = ADC_Channel==1 ? ADC_DEV->Channel1->GetFloat() : ADC_DEV->Channel2->GetFloat();
			// save reading
			trace[counter] = input;
			// apply PID
			setpoint = *((float*)(cmd_buffer+13+4*counter));
			DAC_DEV->WriteFloat(PIDLoop->CalculateFeedback(setpoint, input));
			while(!DAC_DEV->isReady());
		}

		//turn_LED5_on();
		// send result to RPi
		RPi->Transfer(Dummy, (uint8_t*)trace, 4000);
		while(!RPi->isReady());
	}
}

void SelfTest(ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2, RaspberryPi* RPi)
{
	//turn_LED6_on();
	volatile uint32_t countdown;
	uint8_t Dummy[100];
	// communication check
	float* communication_ok = new float[2];
	RPi->Transfer((uint8_t*)communication_ok, Dummy, 8);
	while(!RPi->isReady());
	HAL_Delay(500);
	RPi->Transfer(Dummy, (uint8_t*)communication_ok, 8);
	while(!RPi->isReady());

	// ADC communication check
	HAL_Delay(500);
	uint8_t prev_config_ch1 = ADC_DEV->Channel1->GetConfig();
	uint8_t prev_config_ch2 = ADC_DEV->Channel2->GetConfig();
	//ADC_DEV->Channel1->Setup(ADC_BIPOLAR_10V);
	//ADC_DEV->Channel2->Setup(ADC_BIPOLAR_10V);
	ADC_DEV->Read(); while(!ADC_DEV->isReady());
	ADC_DEV->Read(); while(!ADC_DEV->isReady());
	RPi->Transfer(Dummy, ADC_DEV->getBuffer(), 6);
	while(!RPi->isReady());
	ADC_DEV->Channel1->Setup(prev_config_ch1);
	ADC_DEV->Channel2->Setup(prev_config_ch2);

	DAC_1->WriteFloat(0.0f);
	while(!DAC_1->isReady());
	// DAC 1 communication check
	volatile uint8_t* control_reg;
	HAL_Delay(500);
	while(!DAC_1->isReady());
	DAC_1->MakeDMA2Way();
	DAC_1->ReadControlReg_Step1(); while(!DAC_1->isReady());
	countdown=5;
	while(countdown>0)
		countdown--;
	control_reg = DAC_1->ReadControlReg_Step2(); while(!DAC_1->isReady());
	control_reg[3] = DAC_1->GetControlReg();
	RPi->Transfer(Dummy, (uint8_t*)control_reg, 4);
	while(!RPi->isReady());
	delete control_reg;
	DAC_1->MakeDMA1Way();

	DAC_2->WriteFloat(9.68f);
	while(!DAC_2->isReady());
	// DAC 2 communication check
	HAL_Delay(500);
	while(!DAC_2->isReady());
	//DAC_2->MakeDMA2Way();
	DAC_2->ReadControlReg_Step1(); while(!DAC_2->isReady());
	control_reg = DAC_2->ReadControlReg_Step2(); while(!DAC_2->isReady());
	control_reg[3] = DAC_2->GetControlReg();
	RPi->Transfer(Dummy, (uint8_t*)control_reg, 4);
	while(!RPi->isReady());
	delete control_reg;
	//DAC_2->MakeDMA1Way();
	//turn_LED6_off();
}
