#ifdef CORDIC_TEST_MODULE

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"

/* Reference values in Q1.31 format */
#define ANGLE_CORDIC (int32_t)0x10000000 /* pi/8 in CORDIC input angle mapping */
#define ANGLE_LIB (int32_t)0x08000000    /* pi/8 in arm math lib input angle mapping */
#define MODULUS (int32_t)0x7FFFFFFF      /* 1 */
#define COS_REF (int32_t)0x7641AF3C      /* cos(pi/8) reference value */
#define SIN_REF (int32_t)0x30FBC54D      /* sin(pi/8) reference value */
#define ERROR (uint32_t)0x00001000
#define PASS 0
#define FAIL 1

#define ARRAY_SIZE 64
#define DELTA (int32_t)0x04000000

// #define single_test
#define multi_test

uint32_t Check_Residual_Error(uint32_t VarA, uint32_t VarB, uint32_t MaxError);

int32_t start_angle = 0x00000000;
static uint32_t aAngles[ARRAY_SIZE] =
    {
        0x00000000, 0x04000000, 0x08000000, 0x0C000000,
        0x10000000, 0x14000000, 0x18000000, 0x1C000000,
        0x20000000, 0x24000000, 0x28000000, 0x2C000000,
        0x30000000, 0x34000000, 0x38000000, 0x3C000000,
        0x40000000, 0x44000000, 0x48000000, 0x4C000000,
        0x50000000, 0x54000000, 0x58000000, 0x5C000000,
        0x60000000, 0x64000000, 0x68000000, 0x6C000000,
        0x70000000, 0x74000000, 0x78000000, 0x7C000000,
        0x80000000, 0x84000000, 0x88000000, 0x8C000000,
        0x90000000, 0x94000000, 0x98000000, 0x9C000000,
        0xA0000000, 0xA4000000, 0xA8000000, 0xAC000000,
        0xB0000000, 0xB4000000, 0xB8000000, 0xBC000000,
        0xC0000000, 0xC4000000, 0xC8000000, 0xCC000000,
        0xD0000000, 0xD4000000, 0xD8000000, 0xDC000000,
        0xE0000000, 0xE4000000, 0xE8000000, 0xEC000000,
        0xF0000000, 0xF4000000, 0xF8000000, 0xFC000000};
static uint32_t aRefSin[ARRAY_SIZE] =
    {
        0x00000000, 0x0C8BD35E, 0x18F8B83C, 0x25280C5D,
        0x30FBC54D, 0x3C56BA70, 0x471CECE6, 0x5133CC94,
        0x5A827999, 0x62F201AC, 0x6A6D98A4, 0x70E2CBC6,
        0x7641AF3C, 0x7A7D055B, 0x7D8A5F3F, 0x7F62368F,
        0x80000000, 0x7F62368F, 0x7D8A5F3F, 0x7A7D055B,
        0x7641AF3C, 0x70E2CBC6, 0x6A6D98A4, 0x62F201AC,
        0x5A827999, 0x5133CC94, 0x471CECE6, 0x3C56BA70,
        0x30FBC54D, 0x25280C5D, 0x18F8B83C, 0x0C8BD35E,
        0x00000000, 0xF3742CA2, 0xE70747C4, 0xDAD7F3A3,
        0xCF043AB3, 0xC3A94590, 0xB8E3131A, 0xAECC336C,
        0xA57D8667, 0x9D0DFE54, 0x9592675C, 0x8F1D343A,
        0x89BE50C4, 0x8582FAA5, 0x8275A0C1, 0x809DC971,
        0x80000000, 0x809DC971, 0x8275A0C1, 0x8582FAA5,
        0x89BE50C4, 0x8F1D343A, 0x9592675C, 0x9D0DFE54,
        0xA57D8667, 0xAECC336C, 0xB8E3131A, 0xC3A94590,
        0xCF043AB3, 0xDAD7F3A3, 0xE70747C4, 0xF3742CA2};

int32_t cosOutput = 0;
int32_t sinOutput = 0;
uint32_t start_ticks, stop_ticks, elapsed_ticks;

/* Array of calculated sines in Q1.31 format */
static uint32_t aCalculatedSin[ARRAY_SIZE];
/* Pointer to start of array */
uint32_t *pCalculatedSin = aCalculatedSin;

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
        CORDIC->WDATA = aAngles[0];
        /* Write remaining angles and read sine results */
        for (uint32_t i = 0; i < ARRAY_SIZE; i++)
        {
            // start_angle += DELTA;
            CORDIC->WDATA = aAngles[i];
            *pCalculatedSin++ = CORDIC->RDATA;
        }
        /* Read last result */
        *pCalculatedSin = CORDIC->RDATA;
        stop_ticks = SysTick->VAL;
        elapsed_ticks = start_ticks - stop_ticks;

        /*## Compare CORDIC results to the reference values #####################*/
        for (uint32_t i = 0; i < ARRAY_SIZE; i++)
        {
            if (Check_Residual_Error(aCalculatedSin[i], aRefSin[i], ERROR) == FAIL)
            {
                Error_Handler();
            }
        }

#endif
        while (true)
        {
        }
    }

public:
};

CORDICTestModule *module;

uint32_t Check_Residual_Error(uint32_t VarA, uint32_t VarB, uint32_t MaxError)
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

/******************************
 *         INTERRUPTS          *
 *******************************/
/********************
||      TIM1       ||
********************/

void TIM1_CC_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_CC3IF)
    {
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

    module = new CORDICTestModule();

    module->run();
}
#endif
