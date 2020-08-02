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

#include "benchmarking.hpp"
#include "raspberrypi.hpp"
#include "oscilloscope.hpp"

// device handlers from main.c:
// SPI:
extern SPI_HandleTypeDef hspi1;	// ADC1 & ADC2
extern SPI_HandleTypeDef hspi2;	// extra
extern SPI_HandleTypeDef hspi4;	// Raspberry Pi
extern SPI_HandleTypeDef hspi5;	// DAC2
extern SPI_HandleTypeDef hspi6;	// DAC1

// Timers:
extern TIM_HandleTypeDef htim2;	// Sampling Clock
extern TIM_HandleTypeDef htim3;	// Timeout Clock ADC
extern TIM_HandleTypeDef htim4;	// -


#define SPI_DAC_1	SPI6
#define SPI_DAC_2	SPI5
#define SPI_ADC			SPI1

struct Measurement_Settings {
	uint16_t mode;
	uint16_t n_points;
	uint16_t ADC_Id; // 0 = 1, 1 = 2 , 2=Both
	uint16_t DAC_Id; // 0 = Slow, 1 = Fast , 2=Both
	uint16_t function_Id;
};

typedef struct
{
  __IO uint32_t ISR;   /*!< DMA interrupt status register */
  __IO uint32_t Reserved0;
  __IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
} DMA_Base_Registers;


// Peripheral devices
DAC_Dev *DAC_2, *DAC_1;
ADC_Dev *ADC_DEV;
RaspberryPi *RPi;

PID PIDLoop;
lockparameters* Lock;

// Flags
bool ADC_value_ready = false;
bool Pi_communication_ready = false;
bool locked = false;

// Globals
float* measured_data;
float* measured_data2;

uint16_t sweep_counter = 0;

Measurement_Settings* MS = new Measurement_Settings;

uint16_t* AOM_data;

float scanmin;
float scanmax;


float DAC_Up_V_Slow;
float DAC_Low_V_Slow;
float DAC_Up_V_Fast;
float DAC_Low_V_Fast;

bool ADC1_Bipolar;
bool ADC1_10V;
bool ADC2_Bipolar;
bool ADC2_10V;

bool dummy = false;
int rampcounter = 0;

volatile bool new_data = false;



float set_point_triangle(uint32_t step){
	return TriangleStep(step, scanmin, scanmax, MS->n_points);
}

float set_point_ramp(uint32_t step){
	return RampStep(step, scanmin, scanmax, MS->n_points);
}

float set_point_sine(uint32_t step){
	return SineStep(step, scanmin, scanmax, MS->n_points);
}

float set_point_const(uint32_t step){
	return scanmax;
}

float (*output_func[])(uint32_t) = {set_point_triangle, set_point_ramp, set_point_sine, set_point_const};

float set_point2(uint32_t step){
	return RampStep(step, -10.0f, scanmin, MS->n_points);
}


bool ClockRunning(TIM_HandleTypeDef *htim)
{
	return (((htim->Instance->CR1)&TIM_CR1_CEN) > 0);
}


void SetSamplingFrequency(uint32_t frequency)
{
	HAL_TIM_Base_Stop_IT(&htim2);
	htim2.Init.Period = 90000000/frequency;
	HAL_TIM_Base_Init(&htim2);
	HAL_TIM_Base_Start_IT(&htim2);
}


// This function is called whenever a timer reaches its period
__attribute__((section("sram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

}


// Interrupt for ExtTrigger line
void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
	// Falling Edge = Trigger going High
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_RESET){
		//turn_LED6_on();
		// do something
	}
	// Rising Edge = Trigger going Low
	if(GPIO_Pin == DigitalIn_Pin && HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin)==GPIO_PIN_SET){
		// do something
	}
	// Pi Interrupt pin
	if(GPIO_Pin == Pi_Int_Pin){

		turn_LED1_off();
		turn_LED2_off();
		turn_LED3_off();
		//turn_LED6_on();
		HAL_TIM_Base_Stop_IT(&htim2);
		ADC_value_ready = false;
		Pi_communication_ready = true;
	}

}

void RecieveSettings(SPI_HandleTypeDef *hspi){
	DAC_Up_V_Slow = Pi_ReadFloat(hspi);
	DAC_Low_V_Slow = Pi_ReadFloat(hspi);
	DAC_Up_V_Fast = Pi_ReadFloat(hspi);
	DAC_Low_V_Fast = Pi_ReadFloat(hspi);

	ADC1_Bipolar = (bool)Pi_ReadUInt16(hspi);
	ADC1_10V = (bool)Pi_ReadUInt16(hspi);
	ADC2_Bipolar = (bool)Pi_ReadUInt16(hspi);
	ADC2_10V = (bool)Pi_ReadUInt16(hspi);

	//ADC_1->Setup(ADC1_Bipolar,ADC1_10V);
	//ADC_2->Calibrate(ADC2_Bipolar,ADC2_10V);

	DAC_1->Calibrate(DAC_Up_V_Slow, DAC_Low_V_Slow, false);
	DAC_2->Calibrate(DAC_Up_V_Fast, DAC_Low_V_Fast, false);
}

__attribute__((section("sram_func")))
void DMA2_Stream4_IRQHandler(void)
{
	DAC_2->Callback();
}

__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
	turn_LED5_on();
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
	// SPI 1 tx
	// do nothing
}

__attribute__((section("sram_func")))
void DMA2_Stream0_IRQHandler(void)
{
	// SPI 4 Rx
	RPi->Callback();
}

__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	//RPi->ResetIntPin();
}

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

	// After power on, give all devices a moment to properly start up
	HAL_Delay(200);

	/* Set up input and output devices
	 * The devices communicate with the microcontroller via SPI (serial peripheral interace). For full-speed operation,
	 * a DMA controller (direct memory access) is made responsible for shifting data between SPI registers and RAM on
	 * the microcontroller. See the STM32F4xx reference for more information, in particular for the DMA streams/channels
	 * table.
	 */
	ADC_DEV = new ADC_Dev(/*SPI number*/ 1, /*DMA Stream In*/ 2, /*DMA Channel In*/ 3, /*DMA Stream Out*/ 3, /*DMA Channel Out*/ 3,
			/* conversion pin port*/ ADC_CNV_GPIO_Port, /* conversion pin number*/ ADC_CNV_Pin);
	ADC_DEV->Channel1->Setup(ADC_OFF);
	DAC_1 = new DAC_Dev(/*SPI number*/ 6, /*DMA Stream*/ 5, /*DMA Channel*/ 1, /* sync pin port*/ DAC_1_Sync_GPIO_Port,
			/* sync pin number*/ DAC_1_Sync_Pin, /* clear pin port*/ CLR6_GPIO_Port, /* clear pin number*/ CLR6_Pin);
	DAC_2 = new DAC_Dev(/*SPI number*/ 5, /*DMA Stream*/ 4, /*DMA Channel*/ 2, /* sync pin port*/ DAC_2_Sync_GPIO_Port,
			/* sync pin number*/ DAC_2_Sync_Pin, /* clear pin port*/ CLR5_GPIO_Port, /* clear pin number*/ CLR5_Pin);
	RPi = new RaspberryPi(/*SPI number*/ 4, /*DMA Stream In*/ 0, /*DMA Channel In*/ 4, /*DMA Stream Out*/ 1,
			/*DMA Channel Out*/ 4, /*GPIO Port*/ Pi_Int_GPIO_Port, /*GPIO Pin*/ Pi_Int_Pin);
	//RPi = new RaspberryPi(/*SPI number*/ 4, /*DMA Stream In*/ 0, /*DMA Channel In*/ 4, /*DMA Stream Out*/ 1,
	//			/*DMA Channel Out*/ 4, /*GPIO Port*/ DigitalOut_GPIO_Port, /*GPIO Pin*/ DigitalOut_Pin);


	//HAL_Delay(100);
	turn_LED1_on();
	for(int i=0;i<20;i++){
		DAC_1->Calibrate(10.0, -10.0, false);
		DAC_2->Calibrate(10.0, -10.0, false);
		HAL_Delay(100);
		// TODO: Check how long the delay has to be for correct calibration
	}

	Oscilloscope* Scope = new Oscilloscope(RPi);
	Scope->Setup(200);

	unsigned long sampling_freq = 100; //100000
	//PIDLoop.Setdt(1.0f/sampling_freq);
	//SetSamplingFrequency(sampling_freq);

	//Lock = new lockparameters(&PIDLoop, &htim2, ADC_2, ADC_1, DAC_Slow, DAC_Fast);



	//ADC_Pseudo = new ADC_Dev(&hspi2, ADC_CNV_GPIO_Port, ADC_CNV_Pin, &htim3);
	//ADC_Pseudo = new ADC_Dev(&hspi2, DigitalOut_GPIO_Port, DigitalOut_Pin, &htim3);

	//Pi_communication_ready = true;
	//DAC_Slow->WriteFloat(1.5);
	//DAC_Fast->WriteFloat(3.0);
	uint32_t c = 0;
	volatile int32_t delay;

	//turn_LED5_on();
	//RunAnalogBenchmark(ADC_DEV, DAC_1, DAC_2, 10000UL, &hspi4);
	//turn_LED6_on();
	uint8_t buffer[4];
	uint8_t dummy_buffer[4];
	*((float *)buffer) = 3.1415;
	//HAL_SPI_TransmitReceive_DMA(&hspi4, buffer, dummy_buffer, 4);
	//RPi->Transfer(dummy_buffer, buffer, 4);





	while(1) {
		/*ADC_DEV->Read();
		while(!(ADC_DEV->isReady()));
		DAC_2->WriteFloat(ADC_DEV->Channel1->GetFloat());
		while(!(DAC_2->isReady()));*/

		/*DigitalOut_GPIO_Port->BSRR = ((uint32_t)DigitalOut_Pin)<<16U;
		for(c=0; c<100; c++) {
			ADC_DEV->Read();
			while(!(ADC_DEV->isReady())); }
		DigitalOut_GPIO_Port->BSRR = DigitalOut_Pin;
		for(c=0; c<100; c++) {
			ADC_DEV->Read();
			while(!(ADC_DEV->isReady())); }*/
		//DigitalOut_GPIO_Port->BSRR = ((uint32_t)DigitalOut_Pin)<<16U;
		//DAC_1->WriteFloat(3.0);
		//while(!(DAC_1->isReady()));
		//HAL_Delay(1000);
		//counter++;
		//DigitalOut_GPIO_Port->BSRR = DigitalOut_Pin;
		ADC_DEV->Read();
		while(!ADC_DEV->isReady());
		Scope->Input(ADC_DEV->Channel2->GetFloat());

		//DigitalOut_GPIO_Port->BSRR = ((uint32_t)DigitalOut_Pin)<<16U;
		//ADC_DEV->Read();
		//while(!ADC_DEV->isReady());
		DAC_2->WriteFloat(ADC_DEV->Channel2->GetFloat());
		while(!(DAC_2->isReady()));
		//HAL_Delay(1000);
		//counter++;

		/*ADC_DEV->Read();
		while(!(ADC_DEV->isReady()))
			;
		DAC_Slow->WriteFloat(ADC_DEV->Channel1->GetFloat());
		DAC_2->WriteFloat(ADC_DEV->Channel2->GetFloat());*/
	}

}

