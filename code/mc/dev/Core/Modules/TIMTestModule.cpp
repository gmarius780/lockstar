#ifdef TIM_TEST_MODULE

#include "main.h" 
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"


#define PPM_BUFFER_SIZE ((CAPTURE_BUFFER_SIZE) - 1)
#define PERIOD 0xFFFFF 
#define MEASURED_SIGNAL 1000 /* Measured signal frequency in Hz*/
#define ETRP_CLK 250000 /* external source Clock frequency after the divider in Hz*/
#define TREF ((ETRP_CLK) / (MEASURED_SIGNAL)) /* the ideal number of count between two input capture */
#define CONSTANT ((1000000) / (TREF)) /* constant used to calculate the PPM*/
#define CAPTURE_BUFFER_SIZE ((uint32_t)1001) /* The Buffer size */


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
        prescaler = 34;
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
        while((DMA2->HISR & DMA_HISR_TCIF5) == 0)
        {
        }
        PPM_Calculate();
		while (true)
		{
		}
	}

public:
};

TIMTestModule *module;

static void PPM_Calculate(void)
{
 uint16_t i= 0;
  for (i = 0; i < PPM_BUFFER_SIZE ; i++)
  {
   aPPMBuffer[i] = (TREF - ((aCaptureBuffer[i+1] - aCaptureBuffer[i]))) * CONSTANT;
  }
}

/******************************
 *         INTERRUPTS          *
 *******************************/
/********************
||      DAC1      ||
********************/

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{
	/* To speed up the access to functions, that are often called, we store them in the RAM instead of the FLASH memory.
	 * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
	 * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
	 * */
	extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
	volatile unsigned *src = &__sram_func_loadaddr;
	volatile unsigned *dest = &__sram_func_start;
	while (dest < &__sram_func_end)
	{
		*dest = *src;
		src++;
		dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new TIMTestModule();

	module->run();
}
#endif
