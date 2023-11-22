#ifdef CORDIC_TEST_MODULE

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../../data/angle_data.h"

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

// #define single_test
#define multi_test

uint32_t Check_Residual_Error(int32_t VarA, int32_t VarB, int32_t MaxError);

uint32_t step_size = MAX_VALUE / ARRAY_SIZE;
uint32_t start_angle = 0x00000000;

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
        CORDIC->WDATA = start_angle;
        // CORDIC->WDATA = aAngles[0];
        /* Write remaining angles and read sine results */
        for (uint32_t i = 1; i < ARRAY_SIZE; i++)
        {
            start_angle += step_size;
            CORDIC->WDATA = start_angle;
            // CORDIC->WDATA = aAngles[i];
            *pCalculatedSin++ = CORDIC->RDATA;
        }
        /* Read last result */
        *pCalculatedSin = CORDIC->RDATA;
        stop_ticks = SysTick->VAL;
        elapsed_ticks = start_ticks - stop_ticks;

        /*## Compare CORDIC results to the reference values #####################*/
        for (uint32_t i = 0; i < ARRAY_SIZE; i++)
        {
            if (Check_Residual_Error(aCalculatedSin[i], sin_values[i], ERROR) == FAIL)
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

/******************************
 *         INTERRUPTS          *
 *******************************/
/********************
||      TIM1       ||
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

    module = new CORDICTestModule();

    module->run();
}
#endif
