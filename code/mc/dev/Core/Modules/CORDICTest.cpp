#ifdef CORDIC_TEST_MODULE

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../../data/angle_data.h"

#include "../HAL/ADCDevice.hpp"
#include "../HAL/DACDevice.hpp"
#include "../HAL/BasicTimer.hpp"
#include "dac_config.h"

/* Reference values in Q1.31 format */
#define ANGLE_CORDIC (int32_t)0x10000000 /* pi/8 in CORDIC input angle mapping */
#define ANGLE_LIB (int32_t)0x08000000    /* pi/8 in arm math lib input angle mapping */
#define MODULUS (int32_t)0x7FFFFFFF      /* 1 */
#define COS_REF (int32_t)0x7641AF3C      /* cos(pi/8) reference value */
#define SIN_REF (int32_t)0x30FBC54D      /* sin(pi/8) reference value */
#define ERROR (int32_t)0x00010000
#define PASS 0
#define FAIL 1

#define MAX_VALUE 0xFFFFFFFF
#define FIXED_POINT_FRACTIONAL_BITS 31

// #define single_test
#define multi_test

__STATIC_INLINE float to_float(int32_t value, uint32_t scaling_factor);
uint32_t Check_Residual_Error(int32_t VarA, int32_t VarB, int32_t MaxError);

uint32_t step_size = MAX_VALUE / ARRAY_SIZE;
uint32_t start_angle = 0x00000000;

int32_t cosOutput = 0;
int32_t sinOutput = 0;
uint32_t start_ticks, stop_ticks, elapsed_ticks;

/* Array of calculated sines in Q1.31 format */
static float aCalculatedSin[ARRAY_SIZE];
/* Pointer to start of array */
float *pCalculatedSin = aCalculatedSin;
float *dacPointer = aCalculatedSin;
float *endPointer = aCalculatedSin + (ARRAY_SIZE - 1);

class CORDICTestModule
{
public:
    CORDICTestModule()
    {

        LL_CORDIC_Config(CORDIC,
                         LL_CORDIC_FUNCTION_SINE,     /* cosine function */
                         LL_CORDIC_PRECISION_6CYCLES, /* max precision for q1.31 cosine */
                         LL_CORDIC_SCALE_0,           /* no scale */
                         LL_CORDIC_NBWRITE_1,         /* One input data: angle. Second input data (modulus) is 1
                                  after cordic reset */
                         LL_CORDIC_NBREAD_1,          /* Two output data: cosine, then sine */
                         LL_CORDIC_INSIZE_32BITS,     /* q1.31 format for input data */
                         LL_CORDIC_OUTSIZE_32BITS);
    }
    void run()
    {
        
        dac_1 = new DAC1_Device(&DAC1_conf);
        dac_2 = new DAC2_Device(&DAC2_conf);
        dac_1->config_output();
        dac_2->config_output();

        prescaler = 0;
        counter_max = 549;
        this->sampling_timer = new BasicTimer(2, counter_max, prescaler);

        dac_1->write(0);
        dac_2->write(0);

#ifdef single_test
        start_ticks = SysTick->VAL;
        LL_CORDIC_WriteData(CORDIC, ANGLE_CORDIC);
        cosOutput = (int32_t)LL_CORDIC_ReadData(CORDIC);
        stop_ticks = SysTick->VAL;
        elapsed_ticks = start_ticks - stop_ticks;
#endif
#ifdef multi_test

        start_ticks = SysTick->VAL;
        /* Write first angle to cordic */
        CORDIC->WDATA = start_angle;
        // CORDIC->WDATA = aAngles[0];
        /* Write remaining angles and read sine results */
        for (uint32_t i = 1; i < ARRAY_SIZE; i++)
        {
            // if (i == 500)
            // {
            //     sampling_timer->enable_interrupt();
            //     sampling_timer->enable();
            // }
            start_angle += step_size;
            CORDIC->WDATA = start_angle;
            // CORDIC->WDATA = aAngles[i];
            *pCalculatedSin++ = to_float(CORDIC->RDATA, 5);
        }
        /* Read last result */
        *pCalculatedSin = to_float(CORDIC->RDATA, 5);
        sampling_timer->enable_interrupt();
        sampling_timer->enable();
        // stop_ticks = SysTick->VAL;
        // elapsed_ticks = start_ticks - stop_ticks;

        /*## Compare CORDIC results to the reference values #####################*/
        // for (uint32_t i = 0; i < ARRAY_SIZE; i++)
        // {
        //     if (Check_Residual_Error(aCalculatedSin[i], sin_values[i], ERROR) == FAIL)
        //     {
        //         Error_Handler();
        //     }
        // }

#endif
        while (true)
        {
        }
    }

    void sampling_timer_interrupt()
    {
        // float value = ((float)*(dacPointer++) / (float)(1 << FIXED_POINT_FRACTIONAL_BITS)) * 5;
        if (dacPointer < endPointer)
        {
            this->dac_1->write(*dacPointer);
            this->dac_2->write(*(dacPointer++));
        }

        else
        {
            dacPointer = aCalculatedSin;
            // sampling_timer->disable_interrupt();
            // sampling_timer->disable();
            // stop_ticks = SysTick->VAL;
            // elapsed_ticks = start_ticks - stop_ticks;
        }
    }

public:
    DAC_Device *dac_1;
    DAC_Device *dac_2;
    BasicTimer *sampling_timer;
    uint32_t counter_max, prescaler;
};

CORDICTestModule *module;

uint32_t Check_Residual_Error(int32_t VarA, int32_t VarB, int32_t MaxError)
{
    uint32_t status = PASS;

    if ((VarA - VarB) >= 0)
    {
        if ((VarA - VarB) > MaxError)
        {
            status = FAIL;
        }
    }
    else
    {
        if ((VarB - VarA) > MaxError)
        {
            status = FAIL;
        }
    }

    return status;
}
__STATIC_INLINE float to_float(int32_t value, uint32_t scaling_factor)
{
    return ((float)value / (float)(1 << FIXED_POINT_FRACTIONAL_BITS)) * scaling_factor;
}
/******************************
 *         INTERRUPTS          *
 *******************************/

/********************
||      DAC1      ||
********************/
__attribute__((section (".itcmram")))
void BDMA_Channel1_IRQHandler(void)
{
    module->dac_1->dma_transmission_callback();
}
__attribute__((section (".itcmram")))
void SPI6_IRQHandler(void)
{
    module->dac_1->dma_transmission_callback();
}
/********************
||      DAC2      ||
********************/
__attribute__((section (".itcmram")))
void DMA2_Stream3_IRQHandler(void)
{
    module->dac_2->dma_transmission_callback();
}
__attribute__((section (".itcmram")))
void SPI5_IRQHandler(void)
{
    module->dac_2->dma_transmission_callback();
}
__attribute__((section (".itcmram")))
void TIM2_IRQHandler(void)
{
    LL_TIM_ClearFlag_UPDATE(TIM2);
    module->sampling_timer_interrupt();
}
/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{

    /* After power on, give all devices a moment to properly start up */
    HAL_Delay(200);

    module = new CORDICTestModule();

    module->run();
}
#endif
