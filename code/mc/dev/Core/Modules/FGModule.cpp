#ifdef FG_MODULE

#include "../../data/angle_data.h"
#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"

#include "../HAL/ADCDevice.hpp"
#include "../HAL/BasicTimer.hpp"
#include "../HAL/DACDevice.hpp"
#include "../Lib/BufferBaseModule.h"
#include "../Lib/pid.hpp"
#include "dac_config.h"
#include "etl/circular_buffer.h"
#include <algorithm>

#define FIXED_POINT_FRACTIONAL_BITS 31

__STATIC_INLINE float to_float(int32_t value, float scaling_factor,
                               uint32_t offset);
__STATIC_INLINE void to_bytes(float debug_val);

uint32_t start_ticks, stop_ticks, elapsed_ticks;

/* Array of calculated sines in Q1.31 format */
etl::circular_buffer<float, 18000> aCalculatedSinBuffer;
etl::circular_buffer<float, 18000> bCalculatedSinBuffer;

etl::circular_buffer<uint8_t, 1000> byteBuffer;

etl::icircular_buffer<float>::iterator itr = aCalculatedSinBuffer.begin();
etl::icircular_buffer<float>::iterator itr2 = bCalculatedSinBuffer.begin();

auto end = aCalculatedSinBuffer.begin();
auto end2 = bCalculatedSinBuffer.begin();

__attribute((section(".dtcmram"))) uint16_t chunk_counter = 0;
__attribute((section(".dtcmram"))) uint16_t current_period = 1;
__attribute((section(".dtcmram"))) uint16_t current_period2 = 1;

etl::circular_buffer<waveFunction, 100> functions;
etl::circular_buffer<waveFunction, 100> functions2;
etl::circular_buffer<uint32_t, 100> times_buffer;
etl::circular_buffer<uint32_t, 100> times_buffer2;

uint32_t count = 0;

etl::atomic<bool> unlocked = false;
etl::atomic<bool> unlocked2 = false;
etl::atomic<bool> sample = false;
etl::atomic<bool> sample2 = false;
etl::atomic<bool> idle = false;
etl::atomic<bool> idle2 = false;
std::atomic_flag lock = ATOMIC_FLAG_INIT;

float control = 0;
float c_step_size = 20.0 / 262143; // full_range / (2^20-1)
float c_inv_step_size = 1 / c_step_size;

class FGModule : public BufferBaseModule {
  static const uint32_t BUFFER_LIMIT_kBYTES =
      160; // if this is chosen to large (200) there is no warning, the MC
           // simply crashes (hangs in syscalls.c _exit())
  static const uint32_t MAX_NBR_OF_CHUNKS = 100;

public:
  FGModule() {
    initialize_rpi();
    LL_CORDIC_Config(
        CORDIC, LL_CORDIC_FUNCTION_COSINE, /* cosine function */
        LL_CORDIC_PRECISION_6CYCLES,       /* max precision for q1.31 cosine */
        LL_CORDIC_SCALE_0,                 /* no scale */
        LL_CORDIC_NBWRITE_1,     /* One input data: angle. Second input data
             (modulus) is 1     after cordic reset */
        LL_CORDIC_NBREAD_1,      /* Two output data: cosine, then sine */
        LL_CORDIC_INSIZE_32BITS, /* q1.31 format for input data */
        LL_CORDIC_OUTSIZE_32BITS);
  }
  void run() {

    initialize_adc_dac(ADC_BIPOLAR_10V, ADC_BIPOLAR_10V);

    prescaler = 0;
    counter_max = 1099;

    DBGMCU->APB2FZ1 |= DBGMCU_APB2FZ1_DBG_TIM1; // stop TIM1 when core is halted
    DBGMCU->APB2FZ1 |= DBGMCU_APB2FZ1_DBG_TIM8; // stop TIM8 when core is halted

    DBGMCU->APB1LFZ1 |=
        DBGMCU_APB1LFZ1_DBG_TIM2; // stop TIM2 when core is halted
    DBGMCU->APB1LFZ1 |=
        DBGMCU_APB1LFZ1_DBG_TIM5; // stop TIM2 when core is halted

    TIM1->PSC = 999; // Set prescaler
    TIM1->ARR = 1;

    TIM8->PSC = 27499; // Set prescaler
    TIM8->ARR = 1;

    LL_TIM_EnableARRPreload(TIM1);
    LL_TIM_EnableARRPreload(TIM8);

    this->sampling_timer = new BasicTimer(2, counter_max, prescaler);
    this->sampling_timer2 = new BasicTimer(5, counter_max, prescaler);

    dac_1->write(0);
    dac_2->write(0);

    this->pid_one = new PID(1, 0., 0., 0., 0.);
    this->pid_two = new PID(0., 0., 0., 0., 0.);
    this->setpoint_one = this->setpoint_two = 0.;

    DMA1_Stream7->CR |= DMA_PRIORITY_HIGH;
    DMA1_Stream7->PAR = (uint32_t)&TIM1->DMAR; // Virtual register of TIM1
    DMA1_Stream7->M0AR = (uint32_t)(&times_buffer[0]);

    DMA2_Stream2->CR |= LL_DMA_PRIORITY_MEDIUM;
    DMA2_Stream2->PAR = (uint32_t)&TIM8->DMAR; // Virtual register of TIM8
    DMA2_Stream2->M0AR = (uint32_t)(&times_buffer2[0]);

    LL_TIM_EnableUpdateEvent(TIM1);
    LL_TIM_EnableDMAReq_UPDATE(TIM1);
    LL_TIM_ConfigDMABurst(TIM1, LL_TIM_DMABURST_BASEADDR_ARR,
                          LL_TIM_DMABURST_LENGTH_1TRANSFER);

    LL_TIM_EnableUpdateEvent(TIM8);
    LL_TIM_EnableDMAReq_UPDATE(TIM8);
    LL_TIM_ConfigDMABurst(TIM8, LL_TIM_DMABURST_BASEADDR_ARR,
                          LL_TIM_DMABURST_LENGTH_1TRANSFER);

    LL_TIM_DisableIT_TRIG(sampling_timer->tim_regs);
    LL_TIM_DisableDMAReq_TRIG(sampling_timer->tim_regs);
    LL_TIM_SetSlaveMode(sampling_timer->tim_regs, LL_TIM_SLAVEMODE_TRIGGER);
    LL_TIM_SetTriggerInput(sampling_timer->tim_regs, LL_TIM_TS_ITR0);

    LL_TIM_DisableIT_TRIG(sampling_timer2->tim_regs);
    LL_TIM_DisableDMAReq_TRIG(sampling_timer2->tim_regs);
    LL_TIM_SetSlaveMode(sampling_timer2->tim_regs, LL_TIM_SLAVEMODE_TRIGGER);
    LL_TIM_SetTriggerInput(sampling_timer2->tim_regs, LL_TIM_TS_ITR1);

    while (true) {
      // adc->start_conversion();
      if (sample) {
        sampling_timer_interrupt();
        if (unlocked) {
          dac_1->write();
        }
      }
      if (sample2) {
        sampling_timer_interrupt2();
        if (unlocked2) {
          dac_2->write();
        }
      }
    }
  }

  /*** START: METHODS ACCESSIBLE FROM THE RPI ***/
  static const uint32_t METHOD_SET_CFUNCTION = 31;
  void set_cfunction(RPIDataPackage *read_package) {
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
  void start_ccalculation(RPIDataPackage *read_package) {
    DMA1_Stream7->NDTR = (uint32_t)functions.size();
    LL_TIM_GenerateEvent_UPDATE(TIM1);
    LL_TIM_SetCounter(TIM1, 0);

    DMA2_Stream2->NDTR = (uint32_t)functions2.size();
    LL_TIM_GenerateEvent_UPDATE(TIM8);
    LL_TIM_SetCounter(TIM8, 0);

    for (waveFunction &func : functions) {
      LL_CORDIC_SetFunction(CORDIC, func.function);
      LL_CORDIC_SetScale(CORDIC, func.cordic_scale);
      uint32_t limit = (uint32_t)(func.n_samples * 1) - 1;
      CORDIC->WDATA = func.start_value;
      for (uint32_t j = 1; j < func.n_samples; j++) {
        func.start_value += func.step;
        CORDIC->WDATA = func.start_value;
        float tmp = to_float((int32_t)CORDIC->RDATA, func.scale, func.offset);
        to_bytes(tmp);
        aCalculatedSinBuffer.push(tmp);
        bCalculatedSinBuffer.push(tmp);
      }
      float tmp = to_float((int32_t)CORDIC->RDATA, func.scale, func.offset);
      to_bytes(tmp);
      /* Read last result */
      aCalculatedSinBuffer.push(tmp);
      bCalculatedSinBuffer.push(tmp);
      func.computed = true;
    }

    advance(end, functions.front().n_samples);
    advance(end2, functions2.front().n_samples);

    TIM1->ARR = functions.front().time_start;
    TIM1->CR1 |= TIM_CR1_CEN;
    DMA1_Stream7->CR |= DMA_SxCR_EN; // Enable DMA

    TIM8->ARR = functions2.front().time_start;
    TIM8->CR1 |= TIM_CR1_CEN;
    DMA2_Stream2->CR |= DMA_SxCR_EN; // Enable DMA

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }
  static const uint32_t METHOD_START_Output = 33;
  void start_output(RPIDataPackage *read_package) {
    DMA2_Stream2->NDTR = (uint32_t)functions.size();
    DMA2_Stream2->CR |= DMA_SxCR_EN; // Enable DMA
    TIM8->CR1 |= TIM_CR1_CEN;

    /*** send ACK ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_ack();
    rpi->send_package(write_package);
  }

  void handle_rpi_input() {
    if (handle_rpi_base_methods() ==
        false) { // if base class doesn't know the called method
      /*** Package format: method_identifier (uint32_t) | method specific
       * arguments (defined in the methods directly) ***/
      RPIDataPackage *read_package = this->rpi->get_read_package();
      // switch between method_identifier
      switch (read_package->pop_from_buffer<uint32_t>()) {
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

  void rpi_dma_in_interrupt() {
    if (rpi->dma_in_interrupt()) { /*got new package from rpi*/
      handle_rpi_input();
    } else { /* error */
    }
  }
  void sampling_timer_interrupt() {
    sample = false;
    if (itr < end) {
      unlocked = true;
      itr++;
    } else if (current_period < functions.front().n_periods) {
      current_period++;
      itr = aCalculatedSinBuffer.begin();
    } else if (!functions.empty()) {
      if (functions.front().time_start > 1) {
        sampling_timer->disable();
      }
      aCalculatedSinBuffer.pop(functions.front().n_samples);
      current_period = 1;
      functions.pop();
      times_buffer.pop();
      if (!functions.empty()) {
        advance(end, functions.front().n_samples);
      } else {
        LL_TIM_DisableCounter(TIM1);
        sampling_timer->disable();
        sampling_timer->reset_interrupt();
        LL_TIM_DisableIT_UPDATE(sampling_timer->tim_regs);
        current_period = 1;

        itr = aCalculatedSinBuffer.begin();
        end = aCalculatedSinBuffer.begin();

        TIM1->ARR = 1;
        LL_DMA_ClearFlag_TC7(DMA1);
        DMA1_Stream7->NDTR = 0;
        DMA1_Stream7->PAR = (uint32_t)&TIM1->DMAR; // Virtual register of TIM1
        DMA1_Stream7->M0AR = (uint32_t)(&times_buffer[0]);
        idle = true;
        dac_1->write(0);
      }
    }
  }
  void sampling_timer_interrupt2() {
    sample2 = false;
    if (itr2 < end2) {
      unlocked2 = true;
      itr2++;
    } else if (current_period2 < functions2.front().n_periods) {
      current_period2++;
      itr2 = bCalculatedSinBuffer.begin();
    } else if (!functions2.empty()) {
      if (functions2.front().time_start > 1) {
        sampling_timer2->disable();
      }
      bCalculatedSinBuffer.pop(functions2.front().n_samples);
      current_period2 = 1;
      functions2.pop();
      times_buffer2.pop();
      if (!functions2.empty()) {
        advance(end2, functions2.front().n_samples);
      } else {
        LL_TIM_DisableCounter(TIM8);
        sampling_timer2->disable();
        sampling_timer2->reset_interrupt();
        LL_TIM_DisableIT_UPDATE(sampling_timer2->tim_regs);
        current_period2 = 1;

        itr2 = bCalculatedSinBuffer.begin();
        end2 = bCalculatedSinBuffer.begin();

        TIM8->ARR = 1;
        LL_DMA_ClearFlag_TC2(DMA2);
        DMA2_Stream2->NDTR = 0;
        DMA2_Stream2->PAR = (uint32_t)&TIM8->DMAR; // Virtual register of TIM8
        DMA2_Stream2->M0AR = (uint32_t)(&times_buffer2[0]);
        idle2 = true;
        dac_2->write(0);
      }
    }
  }

public:
  PID *pid_one;
  PID *pid_two;
  float setpoint_one, setpoint_two;
};
__attribute__((section(".dtcmram"))) FGModule *module;

__STATIC_FORCEINLINE float to_float(int32_t value, float scaling_factor,
                                    uint32_t offset) {
  float debug_val =
      ((value * scaling_factor) / (1 << FIXED_POINT_FRACTIONAL_BITS)) + offset;
  return debug_val;
}

__STATIC_FORCEINLINE void to_bytes(float debug_val) {
  uint8_t buffer[3];
  debug_val = std::min((float)10, std::max((float)-10, debug_val));

  int32_t int_output = (int32_t)(debug_val * c_inv_step_size);

  buffer[0] = 0x10;                         // 0x10 first byte
  buffer[0] += ((int_output >> 14) & 0x0f); // Get the most significant 4 bits
  buffer[1] = (int_output >> 6) & 0xff;     // Get the middle 8 bits
  buffer[2] = int_output & 0xff;            // Get the least significant 6 bits

  byteBuffer.push(buffer[0]);
  byteBuffer.push(buffer[1]);
  byteBuffer.push(buffer[2]);
}

/******************************
 *         INTERRUPTS          *
 *******************************/

/********************
||      DAC1      ||
********************/
// __attribute__((section(".itcmram"))) void BDMA_Channel1_IRQHandler(void)
// {
//     module->dac_1->dma_transmission_callback();
// }
// __attribute__((section(".itcmram"))) void SPI6_IRQHandler(void)
// {
//     module->dac_1->dma_transmission_callback();
// }
/********************
||      DAC2      ||
********************/
// __attribute__((section(".itcmram"))) void DMA2_Stream3_IRQHandler(void) {
//   module->dac_2->dma_transmission_callback();
// }
// __attribute__((section(".itcmram"))) void SPI5_IRQHandler(void) {
//   module->dac_2->dma_transmission_callback();
// }
/********************
||       ADC       ||
********************/
__attribute__((section(".itcmram"))) void DMA1_Stream4_IRQHandler(void) {
  module->adc->dma_receive_callback();
}

__attribute__((section(".itcmram"))) void DMA1_Stream5_IRQHandler(void) {
  module->adc->dma_transmission_callback();
}
/********************
||       RPI       ||
********************/
__attribute__((section(".itcmram"))) void DMA1_Stream0_IRQHandler(void) {
  module->rpi_dma_in_interrupt();
}
__attribute__((section(".itcmram"))) void DMA1_Stream1_IRQHandler(void) {
  module->rpi->dma_out_interrupt();
}
__attribute__((section(".itcmram"))) void SPI1_IRQHandler(void) {
  module->rpi->spi_interrupt();
}
/********************
||      Timer      ||
********************/
__attribute__((section(".itcmram"))) void TIM2_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM2);
  // module->sampling_timer_interrupt();
  sample = true;
}
__attribute__((section(".itcmram"))) void TIM5_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM5);
  // module->sampling_timer_interrupt();
  sample2 = true;
}
__attribute__((section(".itcmram"))) void TIM4_IRQHandler(void) {
  LL_TIM_ClearFlag_UPDATE(TIM4);

  // module->rpi->comm_reset_timer_interrupt();
}
__attribute__((section(".itcmram"))) void TIM8_UP_IRQHandler(void) {
  if (TIM8->SR & TIM_SR_UIF) {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    module->enable_sampling();
    TIM8->SR &= ~TIM_SR_UIF;
  }
}
/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void) {

  /* After power on, give all devices a moment to properly start up */
  HAL_Delay(200);

  module = new FGModule();

  module->run();
}
#endif
