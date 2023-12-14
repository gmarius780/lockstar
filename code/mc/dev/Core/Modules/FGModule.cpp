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
// #include "../Src/runtime.h"

#define FIXED_POINT_FRACTIONAL_BITS 31

__STATIC_INLINE float to_float(int32_t value, float scaling_factor, uint32_t offset);

uint32_t start_ticks, stop_ticks, elapsed_ticks;
uint32_t chunke_times_buffer[10] = {5000, 8000, 3000, 4000, 8000, 12000, 3000, 5000, 5000, 2000};
/* Array of calculated sines in Q1.31 format */
static float aCalculatedSin[16384];
/* Pointer to start of array */
float *pCalculatedSin = aCalculatedSin;
float *dacPointer = aCalculatedSin;
float *endPointer;

struct waveFunction
{
    uint32_t function;
    uint32_t cordic_scale;
    int32_t start_value;
    int32_t step;
    uint32_t n_samples;
    float scale;
    uint32_t offset;
    uint32_t n_periods;
};

struct waveFunction functions[4];

class FGModule : public BufferBaseModule
{
    static const uint32_t BUFFER_LIMIT_kBYTES = 160; // if this is chosen to large (200) there is no warning, the MC simply crashes (hangs in syscalls.c _exit())
    static const uint32_t MAX_NBR_OF_CHUNKS = 100;

public:
    FGModule()
    {
        initialize_rpi();
        LL_CORDIC_Config(CORDIC,
                         LL_CORDIC_FUNCTION_COSINE,   /* cosine function */
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

        initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);

        prescaler = 0;
        counter_max = 549;
        this->sampling_timer = new BasicTimer(2, counter_max, prescaler);

        dac_1->write(0);
        dac_2->write(0);

        // allocate buffer and chunk space
        this->buffer = new float[BUFFER_LIMIT_kBYTES * 250]; // contains buffer_one and buffer_two sequentially
        this->chunks = new uint32_t[MAX_NBR_OF_CHUNKS];      // contains chuncks_one and chunks_two sequentially
        functions[0] = {LL_CORDIC_FUNCTION_SINE, LL_CORDIC_SCALE_0, 0, 17179869, 250, 4.0, 0, 4};
        functions[1] = {LL_CORDIC_FUNCTION_ARCTANGENT, LL_CORDIC_SCALE_6, -214748364, 85899, 5000, -568.0519530539988, 4, 2};
        functions[2] = {LL_CORDIC_FUNCTION_SINE, LL_CORDIC_SCALE_0, 0, 17179869, 250, 4.0, 0, 5};
        functions[3] = {LL_CORDIC_FUNCTION_ARCTANGENT, LL_CORDIC_SCALE_6, -214748364, 85899, 5000, -568.0519530539988, 4, 1};

        this->currentFunction = functions[chunk_counter++];
        endPointer = aCalculatedSin + this->currentFunction.n_samples - 1;
        this->current_start = aCalculatedSin;

        DMA1_Stream7->CR |= DMA_PRIORITY_HIGH;
        DMA1_Stream7->NDTR = 10;
        DMA1_Stream7->PAR = (uint32_t)&TIM1->DMAR; // Virtual register of TIM1
        DMA1_Stream7->M0AR = (uint32_t)chunke_times_buffer;

        TIM1->PSC = 10000; // Set prescaler
        TIM1->ARR = 100;

        LL_TIM_EnableARRPreload(TIM1);
        LL_TIM_EnableDMAReq_UPDATE(TIM1);
        LL_TIM_ConfigDMABurst(TIM1, LL_TIM_DMABURST_BASEADDR_ARR, LL_TIM_DMABURST_LENGTH_1TRANSFER);
        LL_TIM_GenerateEvent_UPDATE(TIM1);
        while (!LL_TIM_IsEnabledDMAReq_UPDATE(TIM1))
        {
        }
        LL_TIM_ClearFlag_UPDATE(TIM1);
        LL_TIM_EnableAllOutputs(TIM1);
        LL_TIM_EnableIT_UPDATE(TIM1);

        enable_cycle_counter();
        for (waveFunction func : functions)
        {
            LL_CORDIC_SetFunction(CORDIC, func.function);
            LL_CORDIC_SetScale(CORDIC, func.cordic_scale);
            uint32_t limit = (uint32_t)(func.n_samples * 1) - 1;
            CORDIC->WDATA = func.start_value;
            for (uint32_t i = 1; i < func.n_samples; i++)
            {
                func.start_value += func.step;
                CORDIC->WDATA = func.start_value;
                *pCalculatedSin++ = to_float((int32_t)CORDIC->RDATA, func.scale, func.offset);
            }
            /* Read last result */
            *pCalculatedSin = to_float((int32_t)CORDIC->RDATA, func.scale, func.offset);
        }

        while (true)
        {
        }
    }

    /*** START: METHODS ACCESSIBLE FROM THE RPI ***/
    static const uint32_t METHOD_SET_CFUNCTION = 31;
    void set_cfunction(RPIDataPackage *read_package)
    {
        /***Read arguments***/
        uint32_t func = read_package->pop_from_buffer<uint32_t>();
        uint32_t scale = read_package->pop_from_buffer<uint32_t>();
        LL_CORDIC_SetFunction(CORDIC, func);
        LL_CORDIC_SetScale(CORDIC, scale);

        /*** send ACK ***/
        RPIDataPackage *write_package = rpi->get_write_package();
        write_package->push_ack();
        rpi->send_package(write_package);
    }

    static const uint32_t METHOD_START_CCalculation = 32;
    void start_ccalculation(RPIDataPackage *read_package)
    {
        start_ticks = get_cycle_count();
        /***Read arguments***/
        float scale = read_package->pop_from_buffer<float>();
        uint32_t offset = read_package->pop_from_buffer<uint32_t>();
        uint32_t n_samples = read_package->pop_from_buffer<uint32_t>();
        int32_t start_value = read_package->pop_from_buffer<uint32_t>();
        int32_t step = read_package->pop_from_buffer<uint32_t>();

        endPointer = aCalculatedSin + n_samples - 1;
        uint32_t limit = (uint32_t)(n_samples * 0.1);
        /* Write first angle to cordic */
        CORDIC->WDATA = start_value;
        for (uint32_t i = 1; i < n_samples; i++)
        {
            if (i == limit)
            {
                sampling_timer->enable_interrupt();
                sampling_timer->enable();
            }
            start_value += step;
            CORDIC->WDATA = start_value;
            *pCalculatedSin++ = to_float((int32_t)CORDIC->RDATA, scale, offset);
        }
        /* Read last result */
        *pCalculatedSin = to_float((int32_t)CORDIC->RDATA, scale, offset);

        /*** send ACK ***/
        RPIDataPackage *write_package = rpi->get_write_package();
        write_package->push_ack();
        rpi->send_package(write_package);
    }
    static const uint32_t METHOD_START_Output = 33;
    void start_output(RPIDataPackage *read_package)
    {
        // sampling_timer->enable_interrupt();
        // sampling_timer->enable();
        DMA1_Stream7->CR |= DMA_SxCR_EN; // Enable DMA
        TIM1->CR1 |= TIM_CR1_CEN;

        /*** send ACK ***/
        RPIDataPackage *write_package = rpi->get_write_package();
        write_package->push_ack();
        rpi->send_package(write_package);
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
            case METHOD_SET_CFUNCTION:
                set_cfunction(read_package);
                break;
            case METHOD_START_CCalculation:
                start_ccalculation(read_package);
                break;
            case METHOD_START_Output:
                start_output(read_package);
                break;
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
        else if (this->current_period < this->currentFunction.n_periods)
        {
            this->current_period++;
            dacPointer = this->current_start;
        }
        else if (chunk_counter < 4)
        {
            sampling_timer->disable_interrupt();
            sampling_timer->disable();
            this->current_period = 1;
            this->current_start += this->currentFunction.n_samples;
            this->currentFunction = functions[chunk_counter++];
            endPointer += this->currentFunction.n_samples;
            stop_ticks = get_cycle_count();
            elapsed_ticks = stop_ticks - start_ticks;
        }
        else
        {
            sampling_timer->disable_interrupt();
            sampling_timer->disable();
        }
        // else
        // {
        //     dacPointer = aCalculatedSin;
        // }
    }

public:
    waveFunction currentFunction;
    uint16_t chunk_counter = 0;
    uint16_t current_period = 1;
    float *current_start;
};

FGModule *module;

__STATIC_INLINE float to_float(int32_t value, float scaling_factor, uint32_t offset)
{
    return ((value * scaling_factor) / (1 << FIXED_POINT_FRACTIONAL_BITS)) + offset;
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
__attribute__((section(".itcmram"))) void TIM1_UP_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_UIF)
    {
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
        module->enable_sampling();
        TIM1->SR &= ~TIM_SR_UIF;
    }
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
