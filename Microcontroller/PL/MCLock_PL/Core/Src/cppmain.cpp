#include <adc.hpp>
#include <dac.hpp>
#include <lock.hpp>
#include <main.h>
#include <pid.hpp>
#include <signals.hpp>
#include <spi.h>
#include <leds.hpp>
#include <stm32f427xx.h>
#include <stm32f4xx_hal_gpio.h>
#include <stm32f4xx_hal_spi.h>
#include <stm32f4xx_hal_tim.h>
#include <sys/_stdint.h>
#include "stm32f4xx_it.h"
#include <queue>
#include "benchmarking.hpp"
#include "raspberrypi.hpp"
#include "oscilloscope.hpp"
#include "misc_func.hpp"
#include "flashmemory.hpp"
#include "FFTCorrection.hpp"

#include <math.h>
#include <vector>



// Peripheral devices
DAC_Dev *DAC_2, *DAC_1;
ADC_Dev *ADC_DEV;
RaspberryPi *RPi;
PID* PIDLoop;
FFTCorrection* FFTCorr;
volatile bool new_RPi_input = false;

volatile bool digitalOutOn = false;

volatile bool locking = false;

/* Load saved settings from FLASH to variables */
__attribute__((__section__(".user_data"),used)) uint32_t SavedSettings[4];

volatile bool new_data = false;


//Matrix
struct voltageOut {
    float time;
    float voltage;
};


/************************
 *        TIMER         *
 ************************
 * A timer can be used for time-critical applications. For normal PID operation it is typically sufficient to run the feedback loop as fast as possible,
 * and small fluctuations in the execution time do not matter. In contrast, if sync with the rest of the lab is relevant, then a timer can be employed.
 * It increments a counter with a fixed frequency (90MHz), and calls the below callback function when the counter reaches a pre-set value. */
// reference to the timer
extern TIM_HandleTypeDef htim2;
// set timer frequency and start it
void StartTimer(uint32_t frequency)
{
	HAL_TIM_Base_Stop_IT(&htim2);
	htim2.Init.Period = 90000000/frequency;
	HAL_TIM_Base_Init(&htim2);
	HAL_TIM_Base_Start_IT(&htim2);
}
// stop timer
void StopTimer()
{
	HAL_TIM_Base_Stop_IT(&htim2);
}
// This function is called whenever a timer reaches its period
__attribute__((section("sram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// do something with sensitive timing
	ADC_DEV->Read();

	//For testing the MC-clock rate
	//digitalOutOn = !digitalOutOn;
	//digitalOutOn ? DigitalOutHigh() : DigitalOutLow();
}

//digitalOutOn = !digitalOutOn;
//DAC_1->WriteFloat(digitalOutOn ? 1.0f : -1.0f);


/******************************
 *         CALLBACKS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

// Interrupt for Digital In line (Trigger)
__attribute__((section("sram_func")))
void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{

	//turn_LED6_on();

	// Falling Edge = Trigger going High
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_RESET){
		locking = true;
		PIDLoop->Reset();
		turn_LED6_on();
	}
	// Rising Edge = Trigger going Low
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_SET){
		locking = false;
		turn_LED6_off();
		DAC_2->WriteFloat(DAC_2->GetMin());
	}

}

// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi
__attribute__((section("sram_func")))
void DMA2_Stream4_IRQHandler(void)
{
	DAC_2->Callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
	DAC_1->Callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream2_IRQHandler(void)
{
	// SPI 1 rx
	ADC_DEV->Callback();
	new_data = true;
}
__attribute__((section("sram_func")))
void DMA2_Stream3_IRQHandler(void)
{
	// SPI 1 tx
	// no action required
}
__attribute__((section("sram_func")))
void DMA2_Stream0_IRQHandler(void)
{
	// SPI 4 Rx
	RPi->Callback();
	new_RPi_input = true;
	RPi->ResetIntPin();
}
__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	// no action required
}



//#define NESTED_LOCK
#define CLOCKED_OPERATION


/******************************
 *       MAIN FUNCTION        *
 ******************************/
void cppmain(void)
{
	/* To speed up the access to functions, that are often called, we store them in the RAM instead of the FLASH memory.
	 * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
	 * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
	 * */
	extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
	volatile unsigned *src = &__sram_func_loadaddr;
	volatile unsigned *dest = &__sram_func_start;
	while (dest < &__sram_func_end) {
	  *dest = *src;
	  src++;
	  dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	/* Set up input and output devices
	 * The devices communicate with the microcontroller via SPI (serial peripheral interace). For full-speed operation,
	 * a DMA controller (direct memory access) is made responsible for shifting data between SPI registers and RAM on
	 * the microcontroller. See the STM32F4xx reference for more information, in particular for the DMA streams/channels
	 * table.
	 */
	ADC_DEV = new ADC_Dev(/*SPI number*/ 1, /*DMA Stream In*/ 2, /*DMA Channel In*/ 3, /*DMA Stream Out*/ 3, /*DMA Channel Out*/ 3,
				/* conversion pin port*/ ADC_CNV_GPIO_Port, /* conversion pin number*/ ADC_CNV_Pin);
	//ADC_DEV->Channel1->Setup(ADC_OFF);
	DAC_1 = new DAC_Dev(/*SPI number*/ 6, /*DMA Stream*/ 5, /*DMA Channel*/ 1, /* sync pin port*/ DAC_1_Sync_GPIO_Port,
				/* sync pin number*/ DAC_1_Sync_Pin, /* clear pin port*/ CLR6_GPIO_Port, /* clear pin number*/ CLR6_Pin);
	DAC_2 = new DAC_Dev(/*SPI number*/ 5, /*DMA Stream*/ 4, /*DMA Channel*/ 2, /* sync pin port*/ DAC_2_Sync_GPIO_Port,
			/* sync pin number*/ DAC_2_Sync_Pin, /* clear pin port*/ CLR5_GPIO_Port, /* clear pin number*/ CLR5_Pin);
	RPi = new RaspberryPi(/*SPI number*/ 4, /*DMA Stream In*/ 0, /*DMA Channel In*/ 4, /*DMA Stream Out*/ 1,
			/*DMA Channel Out*/ 4, /*GPIO Port*/ Pi_Int_GPIO_Port, /*GPIO Pin*/ Pi_Int_Pin);

	DigitalOutLow();

	/* Set up oscilloscope function */
	Oscilloscope Scope(RPi, ADC_DEV, DAC_1, DAC_2);
	Scope.AddChannel(OSCILLOSCOPE_REC_ADC1);
	Scope.AddChannel(OSCILLOSCOPE_REC_ADC2);
	//Scope.AddChannel(OSCILLOSCOPE_REC_DAC2);
	Scope.Setup(1.0, 200000.0f);

	/* Set up PID functionality */
	PIDLoop = new PID();
	PIDLoop->max = 10.0;//2.0;
	PIDLoop->min = -10;//1.6;
	PIDLoop->Kp = 0.05;
	PIDLoop->Ki = 0.00000;
	PIDLoop->pol = true;

	/* Set up FFTCorrection functionality */
	vector<peak> peaks = {{1700, 58, 0, 0}}; //{{freq, fftCoeffs, amplitudeShift, phaseShift}}
	//vector<peak> peaks = {{1200.0f, 1, 0.0f, 0.0f}, {1450.0f, 1, 0.0f, 0.0f}, {1700.0f, 1, 0.0f, 0.0f}, {1710.0f, 1, 0.0f, 0.0f}, {1800.0f, 1, 0.0f, 0.0f}, {2000.0f, 1, 0.0f, 0.0f}, {2300.0f, 1, 0.0f, 0.0f}, {2700.0f, 1, 0.0f, 0.0f}, {3003.0f, 12, 0.0f, 0.0f}, {3050, 1, 0.0f, 0.0f}};

	FFTCorr = new FFTCorrection(peaks);
	FFTCorr->SetParameters(10000, 1000);  //Attention: Do not forget to also set the corresponding StartTimer() value in line 230, which represents the sample rate
	FFTCorr->Setup();

	/*Matrix functionality*/
	volatile bool outputVoltage = false;
	vector<voltageOut> voltageOuts = {{1.0f, 1.0f}, {2.0f, 2.0f}};;
	volatile float voltageOutputTime = 0.0;
	volatile uint32_t voltageOutputCounter = 0;
	volatile uint32_t voltageOutputTotal = 2;

	/*vector<float> sineData;
	for (int i = 0; i < 10000; i++) {
		//sineData.push_back(sin(2));
		sineData.push_back(sin((float) (2 * 3.14159265 * 10000 * i) ));
	}*/

#ifdef NESTED_LOCK
	PID* PIDLoop2 = new PID();
	PIDLoop2->Kp = 0.0001;
	PIDLoop2->Ki = 0.0;
	PIDLoop2->Kd = 0.0;
	PIDLoop2->min = -10.0;
	PIDLoop2->max = +10.0;
#endif

	/* Set up DAC output range */
	DAC_1->Setup(DAC_BIPOLAR_10V, false);
	while(!DAC_1->isReady());
	DAC_2->Setup(DAC_BIPOLAR_10V, false);
	while(!DAC_2->isReady());
	DAC_1->WriteFloat(0.0);
	DAC_2->WriteFloat(0.0);
	while(!DAC_1->isReady() && !DAC_2->isReady());

	// set the "power on" indicator LED
	turn_LED5_on();

	DAC_2->SetLimits(0.0f, 5.0f);
	float output = 0.0f;
	float step = 0.005;

#ifdef CLOCKED_OPERATION
	StartTimer(10000);
	float saved_data[10000];
	volatile uint32_t counter=0;
	//float data_temp = new float[1];
	//volatile bool digitalOutOn = false;
	volatile bool flagTemp = true;
#endif

	while(1) {

		/*DAC_1->WriteFloat(output);
		output += step;
		if(output>1.0f or output<0.0f)
			step *= -1.0f;
		while(!DAC_1->isReady());*/

		/* normal main loop operation:
		 * - read from analog-digital converter
		 * - calculate feedback
		 * - write to digital-analog converter
		 * - input data to oscilloscope
		 */

#ifdef CLOCKED_OPERATION

		//if(new_data && counter<10000) {
		if(new_data) {

			if (flagTemp == true) {
				Scope.Input();
			}

			DigitalOutHigh();
			if (outputVoltage) {
				if (voltageOutputCounter == voltageOutputTotal) {
					//turn_LED6_on();
					voltageOutputCounter = 0;
					voltageOutputTime = 0.0;
				}
				DAC_1->WriteFloat((voltageOuts[voltageOutputCounter].voltage * (voltageOuts[voltageOutputCounter+1].time - voltageOutputTime) + voltageOuts[voltageOutputCounter+1].voltage * (voltageOutputTime - voltageOuts[voltageOutputCounter].time)) / (voltageOuts[voltageOutputCounter+1].time - voltageOuts[voltageOutputCounter].time));

				//DAC_1->WriteFloat(((voltageOuts[voltageOutputCounter+1].voltage - voltageOuts[voltageOutputCounter].voltage) / voltageOuts[voltageOutputCounter+1].time) * voltageOutputTime + voltageOuts[voltageOutputCounter].voltage);

				if (voltageOuts[voltageOutputCounter+1].time <= voltageOutputTime) {
					voltageOutputCounter++;
				}

				voltageOutputTime += 1.0/10000.0;
			}
			DigitalOutLow();

			/*if (true) {
				//turn_LED6_on();
				DAC_1->WriteFloat(0.00001 * counter);
				counter++;

				if (counter == 10000) {
					counter = 0;
				}
			}*/

			//DAC_1->WriteFloat(1.0f);

			//digitalOutOn ? DigitalOutHigh() : DigitalOutLow();
			//digitalOutOn = !digitalOutOn;
			//saved_data[counter++] = FFTCorr->CalculateCorrection(ADC_DEV->Channel1->GetFloat());

			//DAC_1->WriteFloat(FFTCorr->CalculateCorrection(ADC_DEV->Channel1->GetFloat()));

			/*DigitalOutHigh();
			DAC_1->WriteFloat(FFTCorr->CalculateCorrection(1.0f));
			//DAC_1->WriteFloat(sin(counter * M_PI / 180));
			//DAC_1->WriteFloat(sineData[counter]);
			DigitalOutLow();*/

			//counter++;

			/*if (digitalOutOn) {
				DigitalOutHigh();
				digitalOutOn = false;
			}*/



			new_data = false;
		} /*else {
			if (!digitalOutOn) {
				DigitalOutLow();
				digitalOutOn = true;
			}
		}*/

		/*if(counter == 10000) {
			//RPi->Write((uint8_t*)saved_data, 4*10000);
			counter = 0;
		}*/
#endif

#ifndef NESTED_LOCK
		/*ADC_DEV->Read();
		while(!ADC_DEV->isReady());
		if(locking)
			DAC_2->WriteFloat(PIDLoop->CalculateFeedback(ADC_DEV->Channel1->GetFloat(), ADC_DEV->Channel2->GetFloat()));
			//PhilipL: DAC_2->WriteFloat(PIDLoop->CalculateFeedback(PIDLoop->set_point, FFTCorr->CalculateCorrection(ADC_DEV->Channel2->GetFloat())));
		Scope.Input();
		while(!(DAC_2->isReady()));*/

		/* in nested operation:
		 * - read from analog-digital converter
		 * - calculate feedback and write to digital-analog converter
		 * - calculate second feedback for other DAC to keep the output of the first at zero
		 * - input data to oscilloscope
		 */

#else
		/*ADC_DEV->Read();
		while(!ADC_DEV->isReady());
		if(locking) {
			DAC_1->WriteFloat(PIDLoop->CalculateFeedback(PIDLoop->set_point, ADC_DEV->Channel2->GetFloat()));
			DAC_2->WriteFloat(PIDLoop2->CalculateFeedback(0.0f, DAC_1->GetLast()));
		}
		Scope.Input();
		while(!(DAC_2->isReady()));*/
#endif


		/* react to commands from the GUI */
		if(new_RPi_input) {
			switch(RPi->ReadBuffer[0]) {

			case RPi_Command_SetScopeSkips:
				Scope.Setup(*((uint16_t*)(RPi->ReadBuffer+1)));
				break;

			case RPi_Command_SetPIDParameters:
				PIDLoop->SetPIDParam((float*)(RPi->ReadBuffer+1));
				break;

			case RPi_Command_MeasureAOMResponse:
			{
				uint8_t ADC_Channel = RPi->ReadBuffer[1];
				uint8_t DAC_Channel = RPi->ReadBuffer[2];
				DAC_2->WriteFloat(0.01);
				while(!DAC_2->isReady());
				HAL_Delay(1000);
				//float* data = RecordTrace(ADC_DEV, ADC_Channel, (DAC_Channel==1) ? DAC_1 : DAC_2, 1000);
				float* data = RecordTrace(ADC_DEV, ADC_Channel==1 ? true : false, ADC_Channel==2 ? true : false, (DAC_Channel==1) ? DAC_1 : DAC_2, 0.01f, 5.0f, 1000);
				// send
				RPi->Transfer(RPi->ReadBuffer, (uint8_t*)data, 4000);
				while(!RPi->isReady());
				// wait until done
				delete data;
				break;
			}

			case RPi_Command_RecordTrace:
			{
				// read parameters
				bool Channel1 = RPi->ReadBuffer[1];
				bool Channel2  = RPi->ReadBuffer[2];
				uint8_t DAC_Channel  = RPi->ReadBuffer[3];
				float From = *((float*)(RPi->ReadBuffer+4));
				float To = *((float*)(RPi->ReadBuffer+8));
				uint32_t Steps = *((uint16_t*)(RPi->ReadBuffer+12));
				// move to starting position
				Move2Voltage((DAC_Channel==1) ? DAC_1 : DAC_2, From);
				// turn low pass on
				/*if (Channel1)
					ADC_DEV->Channel1->SetLowPass(true, 0.9f);
				if (Channel2)
					ADC_DEV->Channel2->SetLowPass(true, 0.9f);*/
				// measure trace
				float* data = RecordTrace(ADC_DEV, Channel1, Channel2, (DAC_Channel==1) ? DAC_1 : DAC_2, From, To, Steps);
				// turn off low pass
				/*if (Channel1)
					ADC_DEV->Channel1->SetLowPass(false, 0.9f);
				if (Channel2)
					ADC_DEV->Channel2->SetLowPass(false, 0.9f);*/
				// send data
				RPi->Write((uint8_t*)data, (Channel1 && Channel2) ? 8*Steps : 4*Steps);
				while(!RPi->isReady());
				// wait until done
				delete data;
				break;
			}

			case RPi_Command_MoveTo:
			{
				// read parameters
				volatile uint8_t DAC_Channel  = RPi->ReadBuffer[1];
				volatile float Pos = *((float*)(RPi->ReadBuffer+2));
				/*float* data = new float[5];
				for(int i=0; i<5; i++)
					data[i] = Pos;
				RPi->Transfer(RPi->ReadBuffer, (uint8_t*)data, 20);*/
				// move to voltage
				Move2Voltage((DAC_Channel==1) ? DAC_1 : DAC_2, Pos);
				/*uint8_t* fakefloat = new uint8_t[4];
				fakefloat[0] = 160;
				fakefloat[1] = 26;
				fakefloat[2] = 231;
				fakefloat[3] = 192;*/
				//DAC_2->WriteFloat(Pos);
				//while(!DAC_2->isReady());
				break;
			}

			case RPi_Command_GotoLock:
			{
				// read parameters
				volatile uint8_t DAC_Channel  = RPi->ReadBuffer[1];
				volatile uint8_t no_waypoints  = RPi->ReadBuffer[2];
				volatile float Step = *((float*)(RPi->ReadBuffer+3));
				volatile bool Flank = RPi->ReadBuffer[7];
				volatile float FeedbackMin = *((float*)(RPi->ReadBuffer+8));
				volatile float FeedbackMax = *((float*)(RPi->ReadBuffer+12));
				volatile float StartFrom = *((float*)(RPi->ReadBuffer+16));
				// transfer waypoints
				float* RawWaypoints = new float[2*no_waypoints];
				float* DummyOutput = new float[2*no_waypoints];
				RPi->Transfer((uint8_t*)RawWaypoints, (uint8_t*)DummyOutput, 8*no_waypoints);
				while(!RPi->isReady());

				// make waypoint queue
				std::queue<Waypoint> Waypoints;
				for(uint16_t i=0; i<no_waypoints; i++)
					Waypoints.push({RawWaypoints[2*i], RawWaypoints[2*i+1]});
				delete RawWaypoints;
				delete DummyOutput;
				// prepare PID
				PIDLoop->pol = Flank;
				//PIDLoop->min = -10;//FeedbackMin;
				//PIDLoop->max = +10;//FeedbackMax;
				PIDLoop->Reset();
				PIDLoop->set_point = Waypoints.back().value;
#ifdef NESTED_LOCK
				PIDLoop2->pol = false;
				PIDLoop2->Reset();
				DAC_1->WriteFloat(0.0);
#endif
				// go to starting position
				//Move2Voltage(DAC_2, StartFrom, 0.00002);
				// go to locking point
				DigitalOutHigh();
				Go2Lock(DAC_2, ADC_DEV, Waypoints, Step);
				// start lock
#ifndef NESTED_LOCK
				PIDLoop->pre_output = DAC_2->GetLast();
#else
				PIDLoop2->pre_output = DAC_2->GetLast();
				PIDLoop->pre_output = 0.0;
#endif
				//ADC_DEV->Channel2->SetLowPass(true, 0.9f);
				DigitalOutLow();
				locking = true;
				turn_LED6_on();
				break;
			}

			case RPi_Command_StopLock:
				locking = false;
				turn_LED6_off();
				break;

			case RPi_Command_StartLock:
				locking = true;
				turn_LED6_on();
				break;

			case RPi_Command_TunePID:
			{
				// read parameters
				volatile uint8_t Procedure = RPi->ReadBuffer[1];
				break;
			}

			case RPi_Command_DetailedTrace:
			{
				// read parameters
				bool Channel1 = RPi->ReadBuffer[1];
				bool Channel2  = RPi->ReadBuffer[2];
				volatile uint8_t DAC_Channel  = RPi->ReadBuffer[3];
				volatile uint32_t From = *((uint32_t*)(RPi->ReadBuffer+4));
				volatile uint32_t NumberSteps = *((uint32_t*)(RPi->ReadBuffer+8));
				volatile uint32_t Blocksize = *((uint32_t*)(RPi->ReadBuffer+12));

				turn_LED5_off();

				DAC_1->WriteFloat(0.0);
				while(!DAC_2->isReady());
				RecordDetailedTrace(ADC_DEV, Channel1, Channel2, (DAC_Channel==1) ? DAC_1 : DAC_2, RPi, From, NumberSteps, Blocksize);
				//TestRCCompensationJump(DAC_2);
				break;
			}

			case RPi_Command_ProgramCalibration:
			{
				// read parameters
				volatile uint8_t DAC_Channel = RPi->ReadBuffer[1];
				volatile float Min = *((float*)(RPi->ReadBuffer+2));
				volatile float Max = *((float*)(RPi->ReadBuffer+6));
				volatile uint32_t NumberPivots = *((uint32_t*)(RPi->ReadBuffer+10));

				// transfer data
				float* Pivots = new float[NumberPivots];
				float* Dummy = new float[NumberPivots];
				RPi->Transfer((uint8_t*)Pivots, (uint8_t*)Dummy, 4*NumberPivots);
				while(!RPi->isReady());

				// calibrate DAC
				DAC_2->Calibrate(Min, Max, Pivots, NumberPivots);

				// clean up
				delete Pivots;
				delete Dummy;
				break;
			}

			case RPi_Command_OptimizePID:
			{
				volatile uint8_t ADC_Channel = RPi->ReadBuffer[1];
				OptimizePID(ADC_DEV, ADC_Channel, DAC_2, RPi, PIDLoop);
				break;
			}

			case RPi_Command_SetFFTCorrectionParameters:
			{
				volatile float sampleRate = *((float*)(RPi->ReadBuffer+1));
				volatile float batchSize = *((float*)(RPi->ReadBuffer+5));
				volatile float nrOfResoanantPeaks = *((float*)(RPi->ReadBuffer+9));

				// For Debug only
				/*if (nrOfResoanantPeaks == 2.0f) {
					turn_LED6_on();
				}*/

				vector<peak> resonantPeaks;
				for (int i=0; i<nrOfResoanantPeaks; i++) {
					resonantPeaks.push_back({
						*((float*)(RPi->ReadBuffer+(13+i*4*4+0))),
						*((float*)(RPi->ReadBuffer+(13+i*4*4+1*4))),
						*((float*)(RPi->ReadBuffer+(13+i*4*4+2*4))),
						*((float*)(RPi->ReadBuffer+(13+i*4*4+3*4)))
					});
				}

				FFTCorr = new FFTCorrection(resonantPeaks);
				FFTCorr->SetParameters(sampleRate, batchSize);  //Attention: Do not forget to also set the corresponding StartTimer() value in line 230, which represents the sample rate
				FFTCorr->Setup();

				// For Debug only
				/*if (FFTCorr->resonantPeaks[1].freq == 1900.0f) {
					turn_LED6_on();
				}*/

				break;
			}
			case RPi_Command_SetChannelEventVoltage:
			{
				flagTemp = false;

				volatile float nrOfVoltageValues = *((float*)(RPi->ReadBuffer+1));
				voltageOutputTotal = (uint32_t) nrOfVoltageValues;
				//voltageOutputTotal = *((uint32_t*)(RPi->ReadBuffer+1));
				//nrOfVoltageValues *= 2; //Since Every voltage values comes in a pair with the corresponding time step in the form (t, V)
				outputVoltage = true;

				//volatile float tempTime = 0.0;
				//volatile float tempVoltage = 0.0;

				//addr = 0x08000000;

				voltageOuts.clear();

				//voltageOuts = vector<voltageOut>(nrOfVoltageValues);

				for (int i=0; i<nrOfVoltageValues; i++) {
					//tempVoltage = *((float*)(RPi->ReadBuffer+(5 + i*4)));

					voltageOuts.push_back({
						*((float*)(RPi->ReadBuffer+(5 + i*4*2 + 0))),
						*((float*)(RPi->ReadBuffer+(5 + i*4*2 + 1*4)))
					});

					//*(__IO uint32_t*)addr = tempVoltage_v2;
					//addr += 4;
//					*(addr++) = (tempVoltage_v2         & 0xF);
//					*(addr++) = ((tempVoltage_v2 >> 8)  & 0xF);
//					*(addr++) = ((tempVoltage_v2 >> 16) & 0xF);
//					*(addr++) = ((tempVoltage_v2 >> 24) & 0xF);

				}

				// For Debug onlya
				/*if (voltageOutputTotal == 8) {
					turn_LED6_on();
				}*/

				/*if (voltageOuts[5].voltage == 1.6f) {
					turn_LED6_on();
				}*/

				float* data_temp = new float[1];
				data_temp[0] = 1997.0;
				RPi->Write((uint8_t*)data_temp, 4*1);
				while(!RPi->isReady());
				delete data_temp;

				flagTemp = false;

				break;
			}

			}
			new_RPi_input = false;
		}
	}

}

