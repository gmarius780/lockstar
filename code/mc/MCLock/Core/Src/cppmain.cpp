#include <lock.hpp>
#include <main.h>
#include <signals.hpp>
#include <stm32f427xx.h>
#include <stm32f4xx_hal_gpio.h>
#include <stm32f4xx_hal_spi.h>
#include <stm32f4xx_hal_tim.h>
#include <sys/_stdint.h>
#include "stm32f4xx_it.h"
#include <queue>
#include "misc_func.hpp"
#include "FFTCorrection.hpp"
#include "../HAL/adc.hpp"
#include "../HAL/dac.hpp"
#include "../HAL/flashmemory.hpp"
#include "../HAL/leds.hpp"
#include "../HAL/raspberrypi.hpp"
#include "../Lib/oscilloscope.hpp"
#include "../Lib/pid.hpp"

#include "../Lib/PIDnew.hpp"
#include "../Lib/PIDsmith.hpp"

// Peripheral devices
DAC_Dev *DAC_2, *DAC_1;
ADC_Dev *ADC_DEV;

/* Declare saved settings from FLASH to variables */
//__attribute__((__section__(".user_data"),used)) uint32_t SavedSettings[4];

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
	//module->trigger_interrupt();
	// Falling Edge = Trigger going High
	/*if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_RESET){
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
	}*/

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
	//module->rpi->Callback();
	//module->rpi_interrupt();
	//module->rpi->ResetIntPin();
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






/******************************
 *       MAIN FUNCTION        *
 ******************************/
void cppmain(void)
{
	/* To speed up the access to functions that are often called, we store them in the RAM instead of the FLASH memory.
	 * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
	 * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
	 * */
	extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
	volatile unsigned *src  = &__sram_func_loadaddr;
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
	ADC_DEV = new ADC_Dev(	/*SPI number*/ 				1,
							/*DMA Stream In*/ 			2,
							/*DMA Channel In*/ 			3,
							/*DMA Stream Out*/ 			3,
							/*DMA Channel Out*/ 		3,
							/* conversion pin port*/ 	ADC_CNV_GPIO_Port,
							/* conversion pin number*/ 	ADC_CNV_Pin);
	ADC_DEV->Channel1->Setup(ADC_UNIPOLAR_10V);
	ADC_DEV->Channel2->Setup(ADC_UNIPOLAR_10V);

	DAC_1 = new DAC_Dev(	/*SPI number*/ 				6,
							/*DMA Stream Out*/ 			5,
							/*DMA Channel Out*/ 		1,
							/*DMA Stream In*/ 			6,
							/*DMA Channel In*/ 			1,
							/* sync pin port*/ 			DAC_1_Sync_GPIO_Port,
							/* sync pin number*/ 		DAC_1_Sync_Pin,
							/* clear pin port*/ 		CLR6_GPIO_Port,
							/* clear pin number*/ 		CLR6_Pin);

	DAC_2 = new DAC_Dev(	/*SPI number*/ 				5,
							/*DMA Stream Out*/ 			4,
							/*DMA Channel Out*/ 		2,
							/*DMA Stream In*/ 			3,
							/*DMA Channel In*/ 			2,
							/* sync pin port*/ 			DAC_2_Sync_GPIO_Port,
							/* sync pin number*/ 		DAC_2_Sync_Pin,
							/* clear pin port*/ 		CLR5_GPIO_Port,
							/* clear pin number*/ 		CLR5_Pin);


	//DAC_1->Setup(DAC_UNIPOLAR_10V, false);
	DAC_1->ConfigOutputs(&hadc3, ADC_CHANNEL_14, ADC_CHANNEL_9);
	DAC_2->ConfigOutputs(&hadc3, ADC_CHANNEL_8, ADC_CHANNEL_15);
	//DAC_2->Setup(DAC_UNIPOLAR_10V, false);
	while(!DAC_1->isReady() && !DAC_2->isReady());

	//DAC_1->SetLimits(0.0f, 10.0f);
	//DAC_2->SetLimits(0.0f, 10.0f);
	while(!DAC_1->isReady() && !DAC_2->isReady());

	DAC_1->WriteFloat(0.0f);
	DAC_2->WriteFloat(0.0f);
	while(!DAC_1->isReady() && !DAC_2->isReady());


	// set the "power on" indicator LED
	//turn_LED5_on();
	//turn_LED6_on();


	/************ TIMER TEST *********************/

	// 1. Enable Peripheral Clock for TIM3 (bit 1 in APB1ENR)
	RCC->APB1ENR |= 1<<1;
	// 2. Set Prescaler to 686 (should be 1s per overflow)
	TIM3->PSC = (uint16_t) 68;
	// 3. Set the Auto Reload Register to 0
	TIM3->ARR = 0xFFFF;
	// 4. Enable update interrupt (bit 0)
	//TIM3->DIER |= 1;
	// 5. Clear interrupt flag
	//TIM3->SR &= 0xFFFE;
	// 6. Enable Counter
	TIM3->CR1 = 1;



	float setpoint = 0;
	float systemOutput = 0;

	// OK-ish tuning for PID controller
	float P = 0.025;
	float I = 11e3;
	float D = 0;

	PIDnew* PIDcontroller = new PIDnew(P,I,D);

	/*********************************************/

	P = 0.025;
	I = 13e3;
	D = 0;

	float dtSim = 1.3e-5;
	int deadtime = 0;
	int order = 2;
	PIDsmith* smithController = new PIDsmith(P,I,D,order,deadtime,dtSim);
	float A[2] = {0.38108794, 0.38108794};
	float B[2] = {1.0, 0.93844733};
	smithController->setModelParameter(A,B,2);

	float controlOutput = 0;

	float dt = 0;
	uint16_t t = TIM3->CNT;
	uint16_t psc = TIM3->PSC;

	bool smith = true;


	while(true) {

		while(!ADC_DEV->isReady());
		ADC_DEV->Read();
		setpoint = ADC_DEV->Channel1->GetFloat();
		systemOutput = ADC_DEV->Channel2->GetFloat();

		t = TIM3->CNT - t;
		dt = t/90e6*psc;

		t = TIM3->CNT;

		if(smith)
			controlOutput = smithController->calcControlOutput(setpoint,systemOutput,dt);
		else
			controlOutput = PIDcontroller->calcControlOutput(setpoint,systemOutput,dt);

		if(controlOutput < 0)
			controlOutput = 0;
		else if(controlOutput > 10)
			controlOutput = 10;

		while(!DAC_1->isReady() && !DAC_2->isReady());
		DAC_2->WriteFloat(controlOutput);
		//DAC_1->WriteFloat(smithController->getLatestModelOutput());
		DAC_1->WriteFloat(setpoint);

		//ADC_DEV->Read();
		//controlOutput = ADC_DEV->Channel1->GetFloat();

		/*
		flag = !flag;
		if(flag)
			DAC_1->WriteFloat(3.0f);
		else
			DAC_1->WriteFloat(0.0f);
		 */


		/* normal main loop operation:
		 * - read from analog-digital converter
		 * - calculate feedback
		 * - write to digital-analog converter
		 * - input data to oscilloscope
		 */

	}
}







