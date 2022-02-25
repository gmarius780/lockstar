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
#include "../HAL/adc.hpp"
#include "../HAL/dac.hpp"



// Peripheral devices
DAC_Dev *DAC_2, *DAC_1;
ADC_Dev *ADC_DEV;
RaspberryPi *RPi;
PID* PIDLoop;
PID* PIDLoop2;
volatile bool new_RPi_input = false;

volatile bool locking = false;

/* Declare saved settings from FLASH to variables */
__attribute__((__section__(".user_data"),used)) uint32_t SavedSettings[4];

volatile bool new_data = false;


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
	//ADC_DEV->Read();
}


extern ADC_HandleTypeDef hadc3;


/******************************
 *         CALLBACKS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

// Interrupt for Digital In line (Trigger)
//sram_func: https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
__attribute__((section("sram_func")))
void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
	// Falling Edge = Trigger going High
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_RESET){
		locking = true;
		PIDLoop->Reset();
		PIDLoop2->Reset();
		PIDLoop2->pre_output=2.5;
		turn_LED6_on();

	}
	// Rising Edge = Trigger going Low
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_SET){
		locking = false;
		turn_LED6_off();
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
	// SPI 1 tx - SPI 5 rx
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
__attribute__((section("sram_func")))
void DMA2_Stream6_IRQHandler(void)
{
	// SPI 6 Rx
	// no action required
}



//#define NESTED_LOCK
//#define CLOCKED_OPERATION
#define SINGLE_PID
//#define DOUBLE_PID


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
	DAC_1 = new DAC_Dev(/*SPI number*/ 6, /*DMA Stream Out*/ 5, /*DMA Channel Out*/ 1, /*DMA Stream In*/ 6, /*DMA Channel In*/ 1, /* sync pin port*/ DAC_1_Sync_GPIO_Port,
			/* sync pin number*/ DAC_1_Sync_Pin, /* clear pin port*/ CLR6_GPIO_Port, /* clear pin number*/ CLR6_Pin);
	DAC_2 = new DAC_Dev(/*SPI number*/ 5, /*DMA Stream Out*/ 4, /*DMA Channel Out*/ 2, /*DMA Stream In*/ 3, /*DMA Channel In*/ 2, /* sync pin port*/ DAC_2_Sync_GPIO_Port,
			/* sync pin number*/ DAC_2_Sync_Pin, /* clear pin port*/ CLR5_GPIO_Port, /* clear pin number*/ CLR5_Pin);
	RPi = new RaspberryPi(/*SPI number*/ 4, /*DMA Stream In*/ 0, /*DMA Channel In*/ 4, /*DMA Stream Out*/ 1,
			/*DMA Channel Out*/ 4, /*GPIO Port*/ Pi_Int_GPIO_Port, /*GPIO Pin*/ Pi_Int_Pin);

	DigitalOutLow();

	/* Set up oscilloscope function */
	Oscilloscope Scope(RPi, ADC_DEV, DAC_1, DAC_2);
	Scope.AddChannel(OSCILLOSCOPE_REC_ADC1);
	Scope.AddChannel(OSCILLOSCOPE_REC_ADC2);
	//Scope.AddChannel(OSCILLOSCOPE_REC_DAC2);
	Scope.Setup(0.01, 200000.0f);

	/* Set up PID functionality */
	PIDLoop = new PID();
	PIDLoop->max = 10.0;//2.0;
	PIDLoop->min = -10;//1.6;
	PIDLoop->Kp = 0.05;
	PIDLoop->Ki = 0.00000;
	PIDLoop->pol = true;

	/* Set up FFTCorrection functionality */
	//FFTCorrection* FFTCorr = new FFTCorrection();
	//FFTCorr->Setup();

#if defined(NESTED_LOCK) || defined(DOUBLE_PID)
	PIDLoop2 = new PID();
	PIDLoop2->Kp = 0.05;//0.0001;
	PIDLoop2->Ki = 0.0;
	PIDLoop2->Kd = 0.0;
	PIDLoop2->min = -10.0;
	PIDLoop2->max = +10.0;
	PIDLoop2->pol = true;
	PIDLoop2->set_point = 0.0;
#endif


	/* Set up DAC output range */
	DAC_1->ConfigOutputs(&hadc3, ADC_CHANNEL_14, ADC_CHANNEL_9);
	//DAC_1->Setup(DAC_BIPOLAR_10V, false);
	//while(!DAC_1->isReady());
	DAC_2->ConfigOutputs(&hadc3, ADC_CHANNEL_8, ADC_CHANNEL_15);
	//DAC_2->Setup(DAC_BIPOLAR_10V, false);
	//while(!DAC_2->isReady());
	DAC_1->WriteFloat(0.0);
	DAC_2->WriteFloat(0.0);
	while(!DAC_1->isReady() && !DAC_2->isReady());

	// set the "power on" indicator LED
	turn_LED5_on();

	//DAC_2->SetLimits(0.0f, 5.0f);
	float output = 0.0;
	float step = 0.005;

#ifdef CLOCKED_OPERATION
	StartTimer(10000);
	float saved_data[20000];
	uint32_t counter=0;
#endif

	while(1) {

		/*DAC_2->WriteFloat(output);
		output += step;
		if(output>4.0f or output<0.0f)
			step *= -1.0f;
		while(!DAC_2->isReady());*/

		/* normal main loop operation:
		 * - read from analog-digital converter
		 * - calculate feedback
		 * - write to digital-analog converter
		 * - input data to oscilloscope
		 */

#ifdef CLOCKED_OPERATION
		if(new_data) {
			saved_data[counter++] = ADC_DEV->Channel1->GetFloat();//FFTCorr->CalculateCorrection(ADC_DEV->Channel1->GetFloat());
			new_data = false;
		}

		if(counter == 20000) {
			RPi->Write((uint8_t*)saved_data, 4*20000);
			counter = 0;
		}
#endif

#if defined(SINGLE_PID)
		ADC_DEV->Read();
		while(!ADC_DEV->isReady());

		if(locking) {
			DigitalOutLow();
			DAC_2->WriteFloat(PIDLoop->CalculateFeedback(ADC_DEV->Channel1->GetFloat(), ADC_DEV->Channel2->GetFloat()));
			//PhilipL: DAC_2->WriteFloat(PIDLoop->CalculateFeedback(PIDLoop->set_point, FFTCorr->CalculateCorrection(ADC_DEV->Channel2->GetFloat())));
		}
		else {
			DAC_2->WriteFloat(0.0);//DAC_2->GetMin());
			DigitalOutHigh();
		}
		/*DAC_2->WriteFloat(output);
		output += step;
		if (output>5.0)
			output = 0.0;
		while(!(DAC_2->isReady()));*/
		/*float* data = RecordTrace(ADC_DEV, true, false, DAC_2, 0.0, 5.0, 1000);
		delete data;*/
		Scope.Input();

#elif defined(DOUBLE_PID)
		ADC_DEV->Read();
		while(!ADC_DEV->isReady());
		if(locking){
			DAC_1->WriteFloat(PIDLoop->CalculateLimitedFeedback(ADC_DEV->Channel1->GetFloat(),0.001));
			DAC_2->WriteFloat(PIDLoop2->CalculateLimitedFeedback(ADC_DEV->Channel2->GetFloat(),0.001));
		}
		else{
			DAC_1->WriteFloat(0.0);
			DAC_2->WriteFloat(2.5);
		}
		Scope.Input();
		while(!(DAC_1->isReady()) or !(DAC_2->isReady()));


		/* in nested operation:
		 * - read from analog-digital converter
		 * - calculate feedback and write to digital-analog converter
		 * - calculate second feedback for other DAC to keep the output of the first at zero
		 * - input data to oscilloscope
		 */

#elif defined(NESTED_LOCK)
		ADC_DEV->Read();
		while(!ADC_DEV->isReady());
		if(locking) {
			DAC_1->WriteFloat(PIDLoop->CalculateFeedback(PIDLoop->set_point, ADC_DEV->Channel2->GetFloat()));
			DAC_2->WriteFloat(PIDLoop2->CalculateFeedback(0.0f, DAC_1->GetLast()));
		}
		Scope.Input();
		while(!(DAC_2->isReady()));
#endif


		/* react to commands from the GUI */
		if(new_RPi_input) {
			switch(RPi->ReadBuffer[0]) {

			case RPi_Command_SetScopeSkips:
				Scope.Setup(*((uint16_t*)(RPi->ReadBuffer+1)));
				break;

			case RPi_Command_SetPIDParameters:
			{
				volatile uint8_t Channel  = RPi->ReadBuffer[1];
				PIDLoop->SetPIDParam((float*)(RPi->ReadBuffer+2));
#if defined(DOUBLE_PID) || defined(NESTED_LOCK)
				PIDLoop2->SetPIDParam((float*)(RPi->ReadBuffer+2));
#endif
				break;
			}

			case RPi_Command_MeasureAOMResponse:
			{
				uint8_t ADC_Channel = RPi->ReadBuffer[1];
				uint8_t DAC_Channel = RPi->ReadBuffer[2];
				DAC_2->Cal->CalibrationOn = false;
				DAC_2->WriteFloat(0.00f);
				while(!DAC_2->isReady());
				ADC_DEV->Read();
				while(!ADC_DEV->isReady());
				HAL_Delay(1000);
				//float* data = RecordTrace(ADC_DEV, ADC_Channel, (DAC_Channel==1) ? DAC_1 : DAC_2, 1000);
				float* data = RecordTrace(ADC_DEV, ADC_Channel==1 ? true : false, ADC_Channel==2 ? true : false, (DAC_Channel==1) ? DAC_1 : DAC_2, 0.0f, 5.0f, 1000);
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

				//turn_LED5_off();

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

			case RPi_Command_SelfTest:
			{
				SelfTest(ADC_DEV, DAC_1, DAC_2, RPi);
				for(int i=0; i<50; i++) {
					DAC_1->WriteFloat(0.0);
					while(!DAC_1->isReady());
					HAL_Delay(50);
					DAC_1->WriteFloat(5.0);
					while(!DAC_1->isReady());
					HAL_Delay(50);
				}
				break;
			}

			case RPi_Command_ProgramSetValue:
			{
				volatile float SetValue1 = *((float*)(RPi->ReadBuffer+1));
				volatile float SetValue2 = *((float*)(RPi->ReadBuffer+5));
				PIDLoop->set_point = SetValue1;
#if defined(DOUBLE_PID) || defined(NESTED_LOCK)
				PIDLoop2->set_point = SetValue2;
#endif
				break;
			}

			case 101:
			{
				float* adc_reads = new float[5];
				adc_reads[0] = DAC_1->V_LOW;
				adc_reads[1] = DAC_1->V_HIGH;
				adc_reads[2] = DAC_2->V_LOW;
				adc_reads[3] = DAC_2->V_HIGH;
				adc_reads[4] = 3.1415;
				float* Dummy = new float[5];
				RPi->Transfer((uint8_t*)Dummy, (uint8_t*)adc_reads, 20);
				while(!RPi->isReady());
			}


			}
			new_RPi_input = false;
		}
	}

}
