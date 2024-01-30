/*
 * BufferBaseModule.h
 *
 *  Created on: Oct 21, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_it.h"

#include "../HAL/BasicTimer.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../Lib/ScopeModule.h"
#include "etl/circular_buffer.h"

#ifndef MODULES_BUFFERBASEMODULE_H_
#define MODULES_BUFFERBASEMODULE_H_

struct waveFunction {
  uint32_t function;
  uint32_t cordic_scale;
  int32_t start_value;
  int32_t step;
  uint32_t n_samples;
  float scale;
  uint32_t offset;
  uint32_t n_periods;
  uint16_t time_start;
};

extern etl::circular_buffer<waveFunction, 100> functions;
extern etl::circular_buffer<uint32_t, 100> times_buffer;
extern etl::circular_buffer<uint32_t, 100> times_buffer2;

class BufferBaseModule : public ScopeModule {
public:
  BufferBaseModule();
  virtual ~BufferBaseModule();

  bool handle_rpi_base_methods() override;

public:
  // if this is chosen too large (200) there is no warning, the MC simply
  // crashes (hangs in syscalls.c _exit()) Had to reduce it in order to make
  // room for the larger rpi-read buffer
  static const uint32_t BUFFER_LIMIT_kBYTES = 140;
  static const uint32_t MAX_NBR_OF_CHUNKS = 100;

  static const uint32_t METHOD_INITIALIZE_BUFFERS = 18;
  void initialize_buffers(RPIDataPackage *read_package);

  static const uint32_t METHOD_SET_SAMPLING_RATE = 19;
  void set_sampling_rate(RPIDataPackage *read_package);

  static const uint32_t METHOD_SET_CH_ONE_CHUNKS = 20;
  void set_ch_one_chunks(RPIDataPackage *read_package);
  static const uint32_t METHOD_SET_CH_TWO_CHUNKS = 21;
  void set_ch_two_chunks(RPIDataPackage *read_package);
  void set_ch_chunks(RPIDataPackage *read_package, uint32_t chunks_size,
                     uint32_t *ch_chunks);

  static const uint32_t METHOD_OUTPUT_ON = 11;
  void output_on(RPIDataPackage *read_package);
  static const uint32_t METHOD_OUTPUT_OFF = 12;
  void output_off(RPIDataPackage *read_package);
  static const uint32_t METHOD_OUTPUT_TTL = 13;
  void output_ttl(RPIDataPackage *read_package);

  static const uint32_t METHOD_SET_CH_ONE_BUFFER = 16;
  void set_ch_one_buffer(RPIDataPackage *read_package);
  static const uint32_t METHOD_SET_CH_TWO_BUFFER = 17;
  void set_ch_two_buffer(RPIDataPackage *read_package);
  void set_ch_buffer(RPIDataPackage *read_package, float *current_read,
                     float *channel_buffer, float *buffer_end, bool buffer_one);

  static const uint32_t METHOD_SET_CH_ONE_FUNC_BUFFER = 22;
  void set_ch_func_buffer(RPIDataPackage *read_package,
                          waveFunction *current_read, waveFunction *func_buffer,
                          uint32_t *time_buffer, bool buffer_one);

  void reset_output();
  void turn_output_off();

  void enable_sampling();
  void disable_sampling();

  bool output_next_chunk();

  bool is_output_on, is_output_ttl;
  uint32_t buffer_one_size, buffer_two_size;
  float *buffer;
  float *buffer_one, *buffer_two;
  float *current_output_one, *current_output_two;
  float *current_read_one,
      *current_read_two; // point to the position where the MC is currently
                         // reading values from the rpi into the buffer
  uint32_t chunks_one_size, chunks_two_size;
  uint32_t *chunks;
  uint32_t *chunks_one, *chunks_two;
  uint32_t *current_chunk_one, *current_chunk_two;
  float *current_end_chunk_one,
      *current_end_chunk_two; // points to the end of the current chunk
  uint32_t counter_max, prescaler;
  BasicTimer *sampling_timer;
  // BasicTimer *sampling_timer2;
  waveFunction *func_buffer_one, *func_buffer_two;
  waveFunction *current_func_one, *current_func_two;
  uint32_t *time_buffer_one, *time_buffer_two;
};

#endif /* MODULES_BUFFERBASEMODULE_H_ */
