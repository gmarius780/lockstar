#ifdef TIM_TEST_MODULE

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../HAL/leds.hpp"

#define PPM_BUFFER_SIZE ((CAPTURE_BUFFER_SIZE)-1)
#define PERIOD 0xFFFFF
#define MEASURED_SIGNAL 10000				  /* Measured signal frequency in Hz*/
#define ETRP_CLK 10000000					  /* external source Clock frequency after the divider in Hz*/
#define TREF ((ETRP_CLK) / (MEASURED_SIGNAL)) /* the ideal number of count between two input capture */
#define CONSTANT ((1000000) / (TREF))		  /* constant used to calculate the PPM*/
#define CAPTURE_BUFFER_SIZE ((uint32_t)1001)  /* The Buffer size */

#define TIM1_DMAR_ADDRESS ((uint32_t)0x4001004c) /* TIM DMAR address for burst access*/
#define BUFFER_DATA_NUMBER ((uint32_t)9)

// uint32_t aSRC_Buffer[BUFFER_DATA_NUMBER] = {4000 - 1, 1, 0, 800, 10000 - 1, 0, 0, 8500, 4000 - 1, 2, 0, 2000};
uint32_t aSRC_Buffer[BUFFER_DATA_NUMBER] = {5000 - 1, 0, 1000, 10000 - 1, 0, 7000, 1000 - 1, 0, 500};
uint16_t Tim1Prescaler = 0;

// #define CH1_FreqMeasure
// #define CH2_PWM
// #define CH3_OCM
// #define TRIGGER
#define TRIGGER_PWM

uint16_t prescaler = 0;
uint16_t aCaptureBuffer[CAPTURE_BUFFER_SIZE];
uint16_t aPPMBuffer[PPM_BUFFER_SIZE];

static void PPM_Calculate(void);

class TIMTestModule
{
public:
	TIMTestModule()
	{
	}
	void run()
	{
#ifdef CH1_FreqMeasure
		// prescaler = 34;
		DMA2_Stream5->CR |= DMA_PRIORITY_HIGH;
		DMA2_Stream5->NDTR = CAPTURE_BUFFER_SIZE;
		DMA2_Stream5->PAR = (uint32_t)&TIM1->CCR1;
		DMA2_Stream5->M0AR = (uint32_t)aCaptureBuffer;
		DMA2_Stream5->CR |= DMA_IT_TE;
		DMA2_Stream5->CR |= DMA_SxCR_EN;
		TIM1->PSC = prescaler;
		TIM1->DIER |= TIM_DIER_CC1DE;
		TIM1->CCER |= TIM_CCER_CC1E;
		TIM1->CR1 |= TIM_CR1_CEN;
		while ((DMA2->HISR & DMA_HISR_TCIF5) == 0)
		{
		}
		PPM_Calculate();
#endif
#ifdef CH2_PWM
		DMA1_Stream7->CR |= DMA_PRIORITY_HIGH;
		DMA1_Stream7->NDTR = BUFFER_DATA_NUMBER;
		DMA1_Stream7->PAR = (uint32_t)&TIM1->DMAR; // Virtual register of TIM1
		DMA1_Stream7->M0AR = (uint32_t)aSRC_Buffer;

		TIM1->CCR2 = 0xFFF;
		LL_TIM_EnableARRPreload(TIM1);	  // Enable ARR Preload
		LL_TIM_EnableDMAReq_UPDATE(TIM1); // Enable DMA request on Update Event
		// Base Address of TIM1 ARR register is used as DMA burst base address
		// Bursts because 4 consecutive register are written (TIM1_ARR, TIM1_RCR TIM1_CCR1, TIM1_CCR2)
		LL_TIM_ConfigDMABurst(TIM1, LL_TIM_DMABURST_BASEADDR_ARR, LL_TIM_DMABURST_LENGTH_4TRANSFERS);
		// Generate an update event to reload the Prescaler and the repetition counter values immediately
		LL_TIM_GenerateEvent_UPDATE(TIM1);
		while (!LL_TIM_IsEnabledDMAReq_UPDATE(TIM1))
		{
		}
		LL_TIM_EnableAllOutputs(TIM1); // Enable all output channels
		TIM1->CCER |= TIM_CCER_CC2E;   // Enable channel 2

#endif
#ifdef CH3_OCM
		LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
		TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_FROZEN;
		TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
		TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
		TIM_OC_InitStruct.CompareValue = 0;
		TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
		TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
		TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
		TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
		LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
		LL_TIM_OC_DisableFast(TIM1, LL_TIM_CHANNEL_CH3);

		TIM1->PSC = 10000; // Set prescaler
		TIM1->CCR3 = 5000;
		LL_TIM_EnableARRPreload(TIM1);
		LL_TIM_GenerateEvent_UPDATE(TIM1);
		LL_TIM_EnableAllOutputs(TIM1);
		LL_TIM_EnableIT_CC3(TIM1);
		TIM1->CCER |= TIM_CCER_CC3E;
		TIM1->CR1 |= TIM_CR1_CEN;
		turn_LED3_on();
#endif
#ifdef TRIGGER

		LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
		TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_FROZEN;
		TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
		TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
		TIM_OC_InitStruct.CompareValue = 0;
		TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
		TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
		TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
		TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
		LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
		LL_TIM_OC_DisableFast(TIM1, LL_TIM_CHANNEL_CH1);

		LL_TIM_SetTriggerInput(TIM1, LL_TIM_TS_TI2FP2);
		LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_TRIGGER);
		LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
		LL_TIM_IC_SetFilter(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV1);
		LL_TIM_IC_SetPolarity(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_RISING);
		LL_TIM_DisableIT_TRIG(TIM1);
		LL_TIM_DisableDMAReq_TRIG(TIM1);
		LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_RESET);
		LL_TIM_SetTriggerOutput2(TIM1, LL_TIM_TRGO2_RESET);

		TIM1->PSC = 10000; // Set prescaler
		TIM1->CCR1 = 5000;

		LL_TIM_SetOnePulseMode(TIM1, LL_TIM_ONEPULSEMODE_SINGLE);
		LL_TIM_EnableARRPreload(TIM1);
		LL_TIM_EnableAllOutputs(TIM1);
		LL_TIM_EnableIT_CC1(TIM1);
		TIM1->CCER |= TIM_CCER_CC1E;
		LL_TIM_GenerateEvent_UPDATE(TIM1);
#endif
#ifdef TRIGGER_PWM

		TIM1->PSC = 1000; // Set prescaler
		TIM1->CCR1 = 0xFFF;
		DMA1_Stream7->CR |= DMA_PRIORITY_HIGH;
		DMA1_Stream7->NDTR = BUFFER_DATA_NUMBER;
		DMA1_Stream7->PAR = (uint32_t)&TIM1->DMAR; // Virtual register of TIM1
		DMA1_Stream7->M0AR = (uint32_t)aSRC_Buffer;

		LL_TIM_SetOnePulseMode(TIM1, LL_TIM_ONEPULSEMODE_SINGLE);

		LL_TIM_SetTriggerInput(TIM1, LL_TIM_TS_TI2FP2);
		LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_TRIGGER);
		LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
		LL_TIM_IC_SetFilter(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV1);
		LL_TIM_IC_SetPolarity(TIM1, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_RISING);
		LL_TIM_DisableIT_TRIG(TIM1);
		LL_TIM_DisableDMAReq_TRIG(TIM1);
		LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_RESET);
		LL_TIM_SetTriggerOutput2(TIM1, LL_TIM_TRGO2_RESET);

		LL_TIM_EnableARRPreload(TIM1);	  // Enable ARR Preload
		LL_TIM_EnableDMAReq_UPDATE(TIM1); // Enable DMA request on Update Event
		// Base Address of TIM1 ARR register is used as DMA burst base address
		// Bursts because 4 consecutive register are written (TIM1_ARR, TIM1_RCR TIM1_CCR1, TIM1_CCR2)
		LL_TIM_ConfigDMABurst(TIM1, LL_TIM_DMABURST_BASEADDR_ARR, LL_TIM_DMABURST_LENGTH_3TRANSFERS);
		// Generate an update event to reload the Prescaler and the repetition counter values immediately
		LL_TIM_GenerateEvent_UPDATE(TIM1);
		while (!LL_TIM_IsEnabledDMAReq_UPDATE(TIM1))
		{
		}
		LL_TIM_EnableAllOutputs(TIM1); // Enable all output channels
		TIM1->CCER |= TIM_CCER_CC1E;   // Enable channel 1
		// TIM1->CR1 |= TIM_CR1_CEN;		 // Enable timer
		DMA1_Stream7->CR |= DMA_SxCR_EN; // Enable DMA

#endif

		while (true)
		{
		}
	}

public:
};

TIMTestModule *module;

static void PPM_Calculate(void)
{
	uint16_t i = 0;
	for (i = 0; i < PPM_BUFFER_SIZE; i++)
	{
		aPPMBuffer[i] = (TREF - ((aCaptureBuffer[i + 1] - aCaptureBuffer[i]))) * CONSTANT;
	}
}

/******************************
 *         INTERRUPTS          *
 *******************************/
/********************
||      TIM1       ||
********************/

__attribute__((section(".itcmram"))) void TIM1_CC_IRQHandler(void)
{
	if (TIM1->SR & TIM_SR_CC3IF)
	{
		turn_LED2_on();
		TIM1->SR &= ~TIM_SR_CC3IF;
	}
	if (TIM1->SR & TIM_SR_CC1IF)
	{
		TIM1->SR &= ~TIM_SR_CC1IF;
	}
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{
	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new TIMTestModule();

	module->run();
}
#endif
