#ifdef FG_MODULE

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../../data/angle_data.h"

#include "../HAL/ADCDevice.hpp"
#include "../HAL/DACDevice.hpp"
#include "../HAL/BasicTimer.hpp"
#include "dac_config.h"
#include "BufferBaseModule.h"


#define MAX_VALUE 0xFFFFFFFF
#define FIXED_POINT_FRACTIONAL_BITS 31

// #define check_cordic_output

__STATIC_INLINE float to_float(int32_t value, uint32_t scaling_factor, uint32_t offset);
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

class FGModule : public BufferBaseModule
{
    static const uint32_t BUFFER_LIMIT_kBYTES = 160; // if this is chosen to large (200) there is no warning, the MC simply crashes (hangs in syscalls.c _exit())
    static const uint32_t MAX_NBR_OF_CHUNKS = 100;

public:
    FGModule()
    {
        initialize_rpi();
        LL_CORDIC_Config(CORDIC,
                         LL_CORDIC_FUNCTION_ARCTANGENT, /* cosine function */
                         LL_CORDIC_PRECISION_6CYCLES,   /* max precision for q1.31 cosine */
                         LL_CORDIC_SCALE_6,             /* no scale */
                         LL_CORDIC_NBWRITE_1,           /* One input data: angle. Second input data (modulus) is 1
                                    after cordic reset */
                         LL_CORDIC_NBREAD_1,            /* Two output data: cosine, then sine */
                         LL_CORDIC_INSIZE_32BITS,       /* q1.31 format for input data */
                         LL_CORDIC_OUTSIZE_32BITS);
    }
    void run()
    {

        initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);

        prescaler = 0;
        counter_max = 549;
        this->sampling_timer = new BasicTimer(2, counter_max, prescaler);

        dac_1->write(0);
        dac_2->write(0);
        /* Write first angle to cordic */
        // CORDIC->WDATA = start_angle;
        CORDIC->WDATA = angle_values[0];
        /* Write remaining angles and read sine results */
        sampling_timer->enable_interrupt();
        sampling_timer->enable();

        // allocate buffer and chunk space
        this->buffer = new float[BUFFER_LIMIT_kBYTES * 250]; // contains buffer_one and buffer_two sequentially
        this->chunks = new uint32_t[MAX_NBR_OF_CHUNKS];      // contains chuncks_one and chunks_two sequentially

        for (uint32_t i = 1; i < ARRAY_SIZE; i++)
        {
            // start_angle += step_size;
            // CORDIC->WDATA = start_angle;
            CORDIC->WDATA = angle_values[i];

            *pCalculatedSin++ = to_float(CORDIC->RDATA, 133.3 * 4, 4);
        }
        /* Read last result */
        *pCalculatedSin = to_float(CORDIC->RDATA, 133.3 * 4, 4);

        while (true)
        {
        }
    }

    void handle_rpi_input()
    {
        if (handle_rpi_base_methods() == false)
        { // if base class doesn't know the called method
            /*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
            RPIDataPackage *read_package = this->rpi->get_read_package();
            // switch between method_identifier
            switch (read_package->pop_from_buffer<uint32_t>())
            {
            default:
                /*** send NACK because the method_identifier is not valid ***/
                RPIDataPackage *write_package = rpi->get_write_package();
                write_package->push_nack();
                rpi->send_package(write_package);
                break;
            }
        }
    }

    /*** START: METHODS ACCESSIBLE FROM THE RPI ***/

    /*** END: METHODS ACCESSIBLE FROM THE RPI ***/

    void rpi_dma_in_interrupt()
    {
        if (rpi->dma_in_interrupt())
        { /*got new package from rpi*/
            handle_rpi_input();
        }
        else
        { /* error */
        }
    }

    void sampling_timer_interrupt()
    {
        if (dacPointer < endPointer)
        {
            this->dac_1->write(*dacPointer);
            this->dac_2->write(*(dacPointer++));
        }

        else
        {
            dacPointer = aCalculatedSin;
        }
    }
};

FGModule *module;

__STATIC_INLINE float to_float(int32_t value, uint32_t scaling_factor, uint32_t offset)
{
    return ((float)value / (float)(1 << FIXED_POINT_FRACTIONAL_BITS)) * scaling_factor + offset;
}
/******************************
 *         INTERRUPTS          *
 *******************************/

/********************
||      DAC1      ||
********************/
__attribute__((section(".itcmram"))) void BDMA_Channel1_IRQHandler(void)
{
    module->dac_1->dma_transmission_callback();
}
__attribute__((section(".itcmram"))) void SPI6_IRQHandler(void)
{
    module->dac_1->dma_transmission_callback();
}
/********************
||      DAC2      ||
********************/
__attribute__((section(".itcmram"))) void DMA2_Stream3_IRQHandler(void)
{
    module->dac_2->dma_transmission_callback();
}
__attribute__((section(".itcmram"))) void SPI5_IRQHandler(void)
{
    module->dac_2->dma_transmission_callback();
}
/********************
||       RPI       ||
********************/
__attribute__((section(".itcmram"))) void DMA1_Stream0_IRQHandler(void)
{
    module->rpi_dma_in_interrupt();
}
__attribute__((section(".itcmram"))) void DMA1_Stream1_IRQHandler(void)
{
    module->rpi->dma_out_interrupt();
}
__attribute__((section(".itcmram"))) void SPI1_IRQHandler(void)
{
    module->rpi->spi_interrupt();
}
/********************
||      Timer      ||
********************/
__attribute__((section(".itcmram"))) void TIM2_IRQHandler(void)
{
    LL_TIM_ClearFlag_UPDATE(TIM2);
    module->sampling_timer_interrupt();
}
__attribute__((section(".itcmram"))) void TIM4_IRQHandler(void)
{
    LL_TIM_ClearFlag_UPDATE(TIM4);
    // module->rpi->comm_reset_timer_interrupt();
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{

    /* After power on, give all devices a moment to properly start up */
    HAL_Delay(200);

    module = new FGModule();

    module->run();
}
#endif
