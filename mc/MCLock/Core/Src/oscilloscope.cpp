/*
 * oscilloscope.cpp
 *
 *  Created on: Mar 6, 2020
 *      Author: qo
 */


#include "raspberrypi.hpp"
#include "oscilloscope.hpp"
#include "adc.hpp"
#include "dac.hpp"
#include "leds.hpp"

Oscilloscope::Oscilloscope(RaspberryPi* RPi, ADC_Dev* ADC_DEV, DAC_Dev* DAC_1, DAC_Dev* DAC_2)
{
	this->RPi = RPi;
	this->ADC_DEV = ADC_DEV;
	this->DAC_1 = DAC_1;
	this->DAC_2 = DAC_2;
	this->DataBuffer = new uint8_t[8*OSCILLOSCOPE_POINTS+20];
	buffer_index = 0;
	skip_counter = 0;
	skips = 200;
}

void Oscilloscope::Setup(uint16_t skips)
{
	buffer_index = 0;
	skip_counter = 0;
	this->skips = skips;
}

__attribute__((section("sram_func")))
void Oscilloscope::Input(float value)
{
	if (delay_counter>0) {
		delay_counter--;
		return;
		;
	}

	if (skip_counter==0) {
		*(((float*)DataBuffer) + buffer_index+5) = value;
		buffer_index++;

		if (buffer_index == OSCILLOSCOPE_POINTS) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer, 4*OSCILLOSCOPE_POINTS+20);
			if ((OSCILLOSCOPE_POINTS-(int32_t)1)*skips < 200000L)
				delay_counter = 200000UL-(OSCILLOSCOPE_POINTS-1UL)*(uint32_t)skips;
		}
		if (buffer_index == 2*OSCILLOSCOPE_POINTS) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer+4*OSCILLOSCOPE_POINTS, 4*OSCILLOSCOPE_POINTS+20);
			buffer_index = 0;
			if ((OSCILLOSCOPE_POINTS-(int32_t)1)*skips < 200000L)
				delay_counter = 200000UL-(OSCILLOSCOPE_POINTS-1UL)*(uint32_t)skips;
		}
	}

	skip_counter++;
	if (skip_counter==skips)
		skip_counter = 0;

}

__attribute__((section("sram_func")))
void Oscilloscope::Input(float value1, float value2)
{
	if (delay_counter>0) {
		delay_counter--;
		return;
		;
	}

	if (skip_counter==0) {
		*(((float*)DataBuffer) + buffer_index+5) = value1;
		*(((float*)DataBuffer) + buffer_index+6) = value2;
		buffer_index+=2;

		if (buffer_index == OSCILLOSCOPE_POINTS) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer, 4*OSCILLOSCOPE_POINTS+20);
			if ((OSCILLOSCOPE_POINTS-(int32_t)1)*skips < 200000L)
				delay_counter = 200000UL-(OSCILLOSCOPE_POINTS-1UL)*(uint32_t)skips;
		}
		if (buffer_index == 2*OSCILLOSCOPE_POINTS) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer+4*OSCILLOSCOPE_POINTS, 4*OSCILLOSCOPE_POINTS+20);
			buffer_index = 0;
			if ((OSCILLOSCOPE_POINTS-(int32_t)1)*skips < 200000L)
				delay_counter = 200000UL-(OSCILLOSCOPE_POINTS-1UL)*(uint32_t)skips;
		}
	}

	skip_counter++;
	if (skip_counter==skips)
		skip_counter = 0;

}

void Oscilloscope::AddChannel(uint32_t Channel)
{
	switch(Channel)
	{
	case OSCILLOSCOPE_REC_ADC1:
		Channels.push_back(&ADC_DEV->Channel1->InputFloat);
		break;
	case OSCILLOSCOPE_REC_ADC2:
		Channels.push_back(&ADC_DEV->Channel2->InputFloat);
		break;
	case OSCILLOSCOPE_REC_DAC1:
		Channels.push_back(&DAC_1->last_output);
		break;
	case OSCILLOSCOPE_REC_DAC2:
		Channels.push_back(&DAC_2->last_output);
		break;
	}
}

void Oscilloscope::Setup(float TimeWindow, float Frequency)
{
	buffer_index = 0;
	skip_counter = 0;
	skips = (uint16_t)(Frequency*TimeWindow*Channels.size()/OSCILLOSCOPE_POINTS-1);
	delay = (uint32_t)((OSCILLOSCOPE_UPDATE_RATE-TimeWindow)*Frequency);
	delay_counter = 0;
	ArrayLimit = (OSCILLOSCOPE_POINTS/Channels.size())*Channels.size();
}

void Oscilloscope::Reset()
{
	buffer_index = 0;
	skip_counter = 0;
	delay_counter = 0;
}

__attribute__((section("sram_func")))
void Oscilloscope::Input()
{
	if (delay_counter>0) {
		delay_counter--;
		return;
		;
	}

	if (skip_counter==0) {
		for(uint16_t index=0; index<Channels.size(); index++){
			volatile float read_number = *(Channels[index]);
			*(((float*)DataBuffer) + buffer_index + 5) = read_number;
			buffer_index++;
		}

		if (buffer_index == ArrayLimit) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer, 4*OSCILLOSCOPE_POINTS+20);
			buffer_index = OSCILLOSCOPE_POINTS;
			delay_counter = delay;
		}
		if (buffer_index == ArrayLimit + OSCILLOSCOPE_POINTS) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer+4*OSCILLOSCOPE_POINTS, 4*OSCILLOSCOPE_POINTS+20);
			buffer_index = 0;
			delay_counter = delay;
		}
	}

	skip_counter++;
	if (skip_counter==skips)
		skip_counter = 0;

}

__attribute__((section("sram_func")))
void Oscilloscope::InputBurst()
{

	if (RPi->isReady()) {
		for(uint16_t index=0; index<Channels.size(); index++){
			volatile float read_number = *(Channels[index]);
			*(((float*)DataBuffer) + buffer_index + 5) = read_number;
			buffer_index++;
		}

		if (buffer_index == ArrayLimit) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer, 4*OSCILLOSCOPE_POINTS+20);
			buffer_index = OSCILLOSCOPE_POINTS;
		}
		if (buffer_index == ArrayLimit + OSCILLOSCOPE_POINTS) {
			RPi->Transfer(RPi->ReadBuffer, DataBuffer+4*OSCILLOSCOPE_POINTS, 4*OSCILLOSCOPE_POINTS+20);
			buffer_index = 0;
		}
	}

}
