/*
 * BufferBaseModule.cpp
 *
 *  Created on: Oct 21, 2022
 *      Author: marius
 */

#include "../Lib/BufferBaseModule.h"

BufferBaseModule::BufferBaseModule() : ScopeModule() {
  // TODO Auto-generated constructor stub
}

BufferBaseModule::~BufferBaseModule() {
  // TODO Auto-generated destructor stub
}

/***
 * read the method-identifier (integer identifying the method e.g.
 * METHOD_OUTPUT_OFF) and call the appropriate method. If the baseModule doesn't
 * recognize the method-identifier, it returns false such that the child-class
 * deals with it.
 *
 * @return: true if it successfully dealt with the method, false otherwise
 */
bool BufferBaseModule::handle_rpi_base_methods() {

  if (ScopeModule::handle_rpi_base_methods() ==
      false) { // if base class doesn't know the called method
    /*** Package format: method_identifier (uint32_t) | method specific
     * arguments (defined in the methods directly) ***/
    RPIDataPackage *read_package = rpi->get_read_package();

    // switch between method_identifier
    switch (read_package->pop_from_buffer<uint32_t>()) {
    case METHOD_OUTPUT_ON:
      output_on(read_package);
      break;
    case METHOD_OUTPUT_OFF:
      output_off(read_package);
      break;
    case METHOD_OUTPUT_TTL:
      output_ttl(read_package);
      break;
    case METHOD_SET_CH_ONE_BUFFER:
      set_ch_one_buffer(read_package);
      break;
    case METHOD_SET_CH_TWO_BUFFER:
      set_ch_two_buffer(read_package);
      break;
    case METHOD_INITIALIZE_BUFFERS:
      initialize_buffers(read_package);
      break;
    case METHOD_SET_SAMPLING_RATE:
      set_sampling_rate(read_package);
      break;
    case METHOD_SET_CH_ONE_CHUNKS:
      set_ch_one_chunks(read_package);
      break;
    case METHOD_SET_CH_TWO_CHUNKS:
      set_ch_two_chunks(read_package);
      break;
    case METHOD_SET_CH_ONE_FUNC_BUFFER:
      set_ch_func_buffer(read_package, this->current_func_one,
                         this->func_buffer_one, this->time_buffer_one, true);
      break;
    default:
      return false;
    }

    return true;
  } else {
    return true;
  }
}

/*** START: METHODS ACCESSIBLE FROM THE RPI ***/

/**
 * set lenths of the two buffer arrays and length of the two chunks arrays. A
 * chunk allows the user to subdivide the buffer into pieces and output them
 * separately.
 */
void BufferBaseModule::initialize_buffers(RPIDataPackage *read_package) {
  this->turn_output_off();
  // set buffers (buffer sizes defined in number of floats)
  buffer_one_size = read_package->pop_from_buffer<uint32_t>();
  buffer_two_size = read_package->pop_from_buffer<uint32_t>();
  chunks_one_size = read_package->pop_from_buffer<uint32_t>();
  chunks_two_size = read_package->pop_from_buffer<uint32_t>();

  prescaler = read_package->pop_from_buffer<uint32_t>();
  counter_max = read_package->pop_from_buffer<uint32_t>();

  if ((buffer_one_size + buffer_two_size > BUFFER_LIMIT_kBYTES * 250) or
      (chunks_one_size + chunks_two_size > MAX_NBR_OF_CHUNKS)) {
    /*** send NACK because requested buffer or chunks is too large ***/
    RPIDataPackage *write_package = rpi->get_write_package();
    write_package->push_nack();
    rpi->send_package(write_package);
    buffer_one_size = 0;
    buffer_two_size = 0;
    chunks_one_size = 0;
    chunks_two_size = 0;

    return;
  } else {
    // set pointers according to the received buffer sizes
    this->reset_output();
    this->sampling_timer->set_auto_reload(counter_max);
    this->sampling_timer->set_prescaler(prescaler);
  }
  this->reset_output();
  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

/**
 * Expects prescaler and counter_max from the datapackage to set the sampling
 * rate: sampling-frq = clock_frq (90e6 Hz) / prescaler / counter_max
 */
void BufferBaseModule::set_sampling_rate(RPIDataPackage *read_package) {
  prescaler = read_package->pop_from_buffer<uint32_t>();
  counter_max = read_package->pop_from_buffer<uint32_t>();

  this->sampling_timer->set_auto_reload(counter_max);
  this->sampling_timer->set_prescaler(prescaler);
  this->sampling_timer->enable_interrupt();

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

/**
 * Reads in the chunks for channel one. The number of chunks has to be set
 * beforehand by calling the method initialize_buffers
 */
void BufferBaseModule::set_ch_one_chunks(RPIDataPackage *read_package) {
  this->set_ch_chunks(read_package, this->chunks_one_size, this->chunks_one);
}

/**
 * Reads in the chunks for channel two. The number of chunks has to be set
 * beforehand by calling the method initialize_buffers
 */
void BufferBaseModule::set_ch_two_chunks(RPIDataPackage *read_package) {
  this->set_ch_chunks(read_package, this->chunks_two_size, this->chunks_two);
}

/**
 * Reads in an array of length <chunks_size> into the chunk_memory at
 * <ch_chunks> from <read_package>
 */
void BufferBaseModule::set_ch_chunks(RPIDataPackage *read_package,
                                     uint32_t chunks_size,
                                     uint32_t *ch_chunks) {
  // chunks are the buffer-parts that should be output together when receiving a
  // trigger an array of integers defines them. Can be used to output a sequence
  // of different waveforms
  this->turn_output_off();

  // read in chunks (number defined with initialize)
  for (uint32_t i = 0; i < chunks_size; i++) {
    ch_chunks[i] = read_package->pop_from_buffer<uint32_t>();
  }

  this->reset_output();

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

/**
 * Starts the output of the next chunk on both outputs, if there is currently
 * still a chunk being output, nothing happens. This method has the same effect
 * as a trigger-input if the output_ttl method has previously been called
 */
void BufferBaseModule::output_on(RPIDataPackage *read_package) {
  // outputs the next chunk
  this->is_output_on = true;
  this->is_output_ttl = false;
  turn_LED6_on();

  this->output_next_chunk();
  this->enable_sampling();

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

/**
 * stop current output, reset all the pointers to point to the first chunk
 */
void BufferBaseModule::output_off(RPIDataPackage *read_package) {
  this->turn_output_off();
  this->reset_output();

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

/**
 * enables ttl: on rising edge at the digital input the next chunk will be
 * output
 */
void BufferBaseModule::output_ttl(RPIDataPackage *read_package) {
  this->is_output_on = false;
  this->is_output_ttl = true;
  turn_LED6_off();

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

/**
 * read in buffer for channel one (see set_ch_buffer for more details)
 */
void BufferBaseModule::set_ch_one_buffer(RPIDataPackage *read_package) {
  this->set_ch_buffer(read_package, this->current_read_one, this->buffer_one,
                      (this->buffer_one + buffer_one_size), true);
}

/**
 * read in buffer for channel two (see set_ch_buffer for more details)
 */
void BufferBaseModule::set_ch_two_buffer(RPIDataPackage *read_package) {
  this->set_ch_buffer(read_package, this->current_read_two, this->buffer_two,
                      (this->buffer_two + buffer_two_size), false);
}

/**
 * Reads in the output-buffer values from the rpi for either buffer_one or
 * buffer_two first: reads a uint32_t from <read_package> to determine
 * nbr_values_to_read second: reads <nbr_values_to_read> into <current_read>
 * which is the current buffer position of either channel one or two
 */
void BufferBaseModule::set_ch_buffer(RPIDataPackage *read_package,
                                     float *current_read, float *channel_buffer,
                                     float *buffer_end, bool buffer_one) {
  this->turn_output_off();

  /***Read arguments***/
  bool append = read_package->pop_from_buffer<bool>();
  uint32_t nbr_values_to_read = read_package->pop_from_buffer<uint32_t>();

  // start filling the buffer from scratch if neccessary
  if (append == false)
    current_read = channel_buffer;

  // read in the given number of values
  float *end_read = current_read + nbr_values_to_read;
  while (current_read < end_read and current_read < buffer_end) {
    *(current_read++) = read_package->pop_from_buffer<float>();
  }

  this->reset_output();

  if (buffer_one == true) {
    this->current_read_one = current_read;
  } else {
    this->current_read_two = current_read;
  }

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}

void BufferBaseModule::set_ch_func_buffer(RPIDataPackage *read_package,
                                          waveFunction *current_read,
                                          waveFunction *func_buffer,
                                          uint32_t *time_buffer,
                                          bool buffer_one) {

  /***Read arguments***/
  uint32_t nbr_values_to_read = read_package->pop_from_buffer<uint32_t>();
  uint32_t time = 0;

  for (uint32_t i = 0; i < nbr_values_to_read; i++) {
    waveFunction temp = {
        .function = read_package->pop_from_buffer<uint32_t>(),
        .cordic_scale = read_package->pop_from_buffer<uint32_t>(),
        .start_value = read_package->pop_from_buffer<int32_t>(),
        .step = read_package->pop_from_buffer<int32_t>(),
        .n_samples = read_package->pop_from_buffer<uint32_t>(),
        .scale = read_package->pop_from_buffer<float>(),
        .offset = read_package->pop_from_buffer<uint32_t>(),
        .n_periods = read_package->pop_from_buffer<uint32_t>()};
    time = read_package->pop_from_buffer<uint32_t>();
    temp.time_start = time;
    times_buffer.push(time);
    times_buffer2.push(time);
    // *(time_buffer) = time;
    functions.push(temp);
    // time_buffer++;
  }

  /*** send ACK ***/
  RPIDataPackage *write_package = rpi->get_write_package();
  write_package->push_ack();
  rpi->send_package(write_package);
}
/*** END: METHODS ACCESSIBLE FROM THE RPI ***/

/**
 * Reset all the pointers to their start values such that at the next
 * output_on/trigger the first chunks will be output
 */
void BufferBaseModule::reset_output() {
  // set buffers and chunks to the starting point such that at the next trigger
  // the first chunk is output
  buffer_one = buffer;
  buffer_two = buffer_one + buffer_one_size;
  current_output_one = buffer_one;
  current_output_two = buffer_two;
  current_end_chunk_one = buffer_one;
  current_end_chunk_two = buffer_two;
  chunks_one = chunks;
  chunks_two = chunks_one + chunks_one_size;
  current_chunk_one = chunks_one;
  current_chunk_two = chunks_two;
}

/**
 * disable output by outputting 0
 */
void BufferBaseModule::turn_output_off() {
  this->disable_sampling();
  this->is_output_on = false;
  this->is_output_ttl = false;
  dac_1->write(0.);
  dac_2->write(0.);
  turn_LED6_off();
}

void BufferBaseModule::enable_sampling() {
  this->sampling_timer->enable_interrupt();
  this->sampling_timer->enable();
}

void BufferBaseModule::disable_sampling() {
  this->sampling_timer->disable_interrupt();
  this->sampling_timer->disable();
}

/**
 * If there is currently no chunk being output, it sets the pointers such that
 * the next chunk is starting to be sampled
 */
bool BufferBaseModule::output_next_chunk() {
  // if we are still outputting
  if (current_output_one < current_end_chunk_one or
      current_output_two < current_end_chunk_two) {
    return false;
  } else {
    // The chunk 'array' goes from chunks_one to chunks_one + chunks_one_size
    // - 1. It contains the buffer-indices which correspond to the end of the
    // respective chunk meaning: the values of the n-th chunk in the buffer are:
    // buffer[chunks_one[n-1]] ... buffer[chunks_one[n]-1]
    this->disable_sampling();
    // set current_output_one to start of current chunk and
    // current_end_chunk_one to end
    if (current_chunk_one >=
        chunks_one + chunks_one_size) { // check if last chunk is reached
      current_chunk_one = chunks_one;
    }

    if (current_chunk_one == chunks_one) { // first chunk
      current_output_one = buffer_one;
    } else { // n-th chunk
      current_output_one = buffer_one + *(current_chunk_one - 1) + 1;
    }
    current_end_chunk_one = buffer_one + *(current_chunk_one++);

    if (current_chunk_two >=
        chunks_two + chunks_two_size) { // check if last chunk is reached
      current_chunk_two = chunks_two;
    }
    if (current_chunk_two == chunks_two) { // first chunk
      current_output_two = buffer_two;
    } else { // n-th chunk
      current_output_two = buffer_two + *(current_chunk_two - 1) + 1;
    }
    current_end_chunk_two = buffer_two + *(current_chunk_two++);

    // currently_outputting_chunk_one = currently_outputting_chunk_two = true;
    turn_LED6_on();
    this->enable_sampling();
    return true;
  }
}
