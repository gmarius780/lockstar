/*
 * AWGModule.cpp
 *
 *
 *  Created on: Aug 18, 2022
 *      Author: marius
 */
#include "main.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_hal_gpio.h"


#include "../HAL/spi.hpp"
#include "../HAL/rpi.h"
#include "../HAL/leds.hpp"
#include "../HAL/adc_new.hpp"
#include "../HAL/dac_new.hpp"
#include "../HAL/basic_timer.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../Lib/pid.hpp"

#include "Module.hpp"

#ifdef AWG_MODULE



class AWGModule: public Module {
	static const uint32_t BUFFER_LIMIT_kBYTES = 180; //if this is chosen to large (200) there is no warning, the MC simply crashes (hangs in syscalls.c _exit())
	static const uint32_t MAX_NBR_OF_CHUNKS = 100;
public:
	AWGModule() {
		initialize_rpi();
		turn_LED6_off();
		turn_LED5_on();
		//default sampling rate is INTERNAL_CLOCK_FREQUENCY/prescaler * counter_max = 90e6/90*1000 = 1khz
		prescaler=90;
		counter_max=1000;
		this->sampling_timer = new BasicTimer(2, counter_max, prescaler, false);

		// allocate buffer and chunk space
		this->buffer = new float[BUFFER_LIMIT_kBYTES*250]; //contains buffer_one and buffer_two sequencially
		this->chunks = new uint32_t[MAX_NBR_OF_CHUNKS]; //contains chuncks_one and chunks_two sequencially
		currently_outputting_chunk_one = currently_outputting_chunk_two = false;
	}

	void run() {
		initialize_adc(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
		initialize_dac();
		this->dac_1->write(0);
		this->dac_2->write(0);

		/*** work loop ***/
		while(true) {
			//this->dac_1->write(this->pid->calculate_output(adc->channel1->get_result(), adc->channel2->get_result(), dt));
		}
	}

	void handle_rpi_input() {
		/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
		RPIDataPackage* read_package = rpi->get_read_package();

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
		case METHOD_SET_CH_ONE_OUTPUT_LIMITS:
			set_ch_one_output_limits(read_package);
			break;
		case METHOD_SET_CH_TWO_OUTPUT_LIMITS:
			set_ch_two_output_limits(read_package);
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
		default:
			/*** send NACK because the method_identifier is not valid ***/
			RPIDataPackage* write_package = rpi->get_write_package();
			write_package->push_nack();
			rpi->send_package(write_package);
			break;
		}

	}

	/*** START: METHODS ACCESSIBLE FROM THE RPI ***/
	static const uint32_t METHOD_INITIALIZE_BUFFERS = 18;
	void initialize_buffers(RPIDataPackage* read_package) {
		this->turn_output_off();
		//set buffers (buffer sizes defined in number of floats)
		buffer_one_size = read_package->pop_from_buffer<uint32_t>();
		buffer_two_size = read_package->pop_from_buffer<uint32_t>();
		chunks_one_size = read_package->pop_from_buffer<uint32_t>();
		chunks_two_size = read_package->pop_from_buffer<uint32_t>();
		prescaler = read_package->pop_from_buffer<uint32_t>();
		counter_max = read_package->pop_from_buffer<uint32_t>();

		if ((buffer_one_size + buffer_two_size > BUFFER_LIMIT_kBYTES*250) or (chunks_one_size + chunks_two_size > MAX_NBR_OF_CHUNKS)) {
			/*** send NACK because requested buffer or chunks is too large ***/
			RPIDataPackage* write_package = rpi->get_write_package();
			write_package->push_nack();
			rpi->send_package(write_package);
		} else {
			//set pointers according to the received buffer sizes
			this->reset_output();
			this->sampling_timer->set_auto_reload(counter_max);
			this->sampling_timer->set_prescaler(prescaler);
		}

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_SAMPLING_RATE = 19;
	void set_sampling_rate(RPIDataPackage* read_package) {
		prescaler = read_package->pop_from_buffer<uint32_t>();
		counter_max = read_package->pop_from_buffer<uint32_t>();

		this->turn_output_off();
		this->reset_output();
		this->sampling_timer->set_auto_reload(counter_max);
		this->sampling_timer->set_prescaler(prescaler);

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_CH_ONE_CHUNKS = 20;
	void set_ch_one_chunks(RPIDataPackage* read_package) {
		this->set_ch_chunks(read_package, this->chunks_one_size, this->chunks_one);
	}

	static const uint32_t METHOD_SET_CH_TWO_CHUNKS = 21;
	void set_ch_two_chunks(RPIDataPackage* read_package) {
		this->set_ch_chunks(read_package, this->chunks_two_size, this->chunks_two);
	}

	void set_ch_chunks(RPIDataPackage* read_package, uint32_t chunks_size, uint32_t *ch_chunks) {
		//chunks are the buffer-parts that should be output together when receiving a trigger
		//an array of integers defines them. Can be used to output a sequence of different waveforms
		this->turn_output_off();

		//read in chunks (number defined with initialize)
		for(uint32_t i = 0; i < chunks_size; i++) {
			ch_chunks[i] = read_package->pop_from_buffer<uint32_t>();
		}

		this->reset_output();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_OUTPUT_ON = 11;
	void output_on(RPIDataPackage* read_package) {
		//outputs the next chunk
		this->is_output_on = true;
		this->is_output_ttl = false;
		turn_LED6_on();

		this->output_next_chunk();
		this->enable_sampling();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_OUTPUT_OFF = 12;
	void output_off(RPIDataPackage* read_package) {
		this->turn_output_off();
		this->reset_output();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_OUTPUT_TTL = 13;
	void output_ttl(RPIDataPackage* read_package) {
		this->is_output_on = false;
		this->is_output_ttl = true;
		turn_LED6_off();

		this->enable_sampling();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_CH_ONE_OUTPUT_LIMITS = 14;
	void set_ch_one_output_limits(RPIDataPackage* read_package) {
		set_ch_output_limit(read_package, this->dac_1);
	}

	static const uint32_t METHOD_SET_CH_TWO_OUTPUT_LIMITS = 15;
	void set_ch_two_output_limits(RPIDataPackage* read_package) {
		set_ch_output_limit(read_package, this->dac_2);
	}

	static const uint32_t METHOD_SET_CH_ONE_BUFFER = 16;
	void set_ch_one_buffer(RPIDataPackage* read_package) {
		this->set_ch_buffer(read_package, this->current_read_one, this->buffer_one, this->buffer_one + buffer_one_size);
	}

	static const uint32_t METHOD_SET_CH_TWO_BUFFER = 17;
	void set_ch_two_buffer(RPIDataPackage* read_package) {
		this->set_ch_buffer(read_package, this->current_read_two, this->buffer_two, this->buffer_two + buffer_two_size);
	}

	void set_ch_buffer(RPIDataPackage* read_package, float *current_read, float *channel_buffer, float *buffer_end) {
		this->turn_output_off();

		/***Read arguments***/
		bool append = read_package->pop_from_buffer<bool>();
		uint32_t nbr_values_to_read = read_package->pop_from_buffer<uint32_t>();

		// start filling the buffer from scratch if neccessary
		if (append == false)
			current_read = channel_buffer;

		//read in the given number of values
		while (current_read < current_read + nbr_values_to_read and current_read < buffer_end) {
			*(current_read++) = read_package->pop_from_buffer<float>();
		}

		this->reset_output();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	/*** END: METHODS ACCESSIBLE FROM THE RPI ***/

	void reset_output() {
		//set buffers and chunks to the starting point such that at the next trigger the first chunk is output
		buffer_one = buffer;
		buffer_two = buffer_one + buffer_one_size;
		current_output_one = buffer_one;
		current_output_two = buffer_two;
		chunks_one = chunks;
		chunks_two = chunks_one + chunks_one_size;
		current_chunk_one = chunks_one;
		current_chunk_two = chunks_two;
	}

	void turn_output_off() {
		this->disable_sampling();
		this->is_output_on = false;
		this->is_output_ttl = false;
		this->dac_1->write(0);
		this->dac_2->write(0);
		turn_LED6_off();
	}

	void enable_sampling() {
		this->sampling_timer->enable_interrupt();
		this->sampling_timer->enable();
	}

	void disable_sampling() {
		this->sampling_timer->disable_interrupt();
		this->sampling_timer->disable();
	}

	bool output_next_chunk() {
		//if we are still outputting
		if (currently_outputting_chunk_one == true or currently_outputting_chunk_two == true) {
			return false;
		} else {
			//The chunk 'array' goes from chunks_one to chunks_one + chunks_one_size - 1.
			//It contains the buffer-indices which correspond to the end of the respective chunk
			//meaning: the values of the n-th chunk in the buffer are: buffer[chunks_one[n-1]] ... buffer[chunks_one[n]-1]
			this->disable_sampling();
			//set current_output_one to start of current chunk and current_end_chunk_one to end
			if (current_chunk_one >= chunks_one + chunks_one_size) { // check if last chunk is reached
				current_chunk_one = chunks_one;
			}

			if (current_chunk_one == chunks_one) {//first chunk
				current_output_one = buffer_one;
			} else { //n-th chunk
				current_output_one = buffer_one + *(current_chunk_one-1);
			}
			current_end_chunk_one = buffer_one + *(current_chunk_one++);

			if (current_chunk_two >= chunks_two + chunks_two_size) { // check if last chunk is reached
				current_chunk_two = chunks_two;
			}
			if (current_chunk_two == chunks_two) {//first chunk
				current_output_two = buffer_two;
			} else { //n-th chunk
				current_output_two = buffer_two + *(current_chunk_two-1);
			}
			current_end_chunk_two = buffer_two + *(current_chunk_two++);

			//
			currently_outputting_chunk_one = currently_outputting_chunk_two = true;
			turn_LED6_on();
			this->enable_sampling();
			return true;
		}
	}

	void rpi_dma_in_interrupt() {

		if(rpi->dma_in_interrupt())
		{ /*got new package from rpi*/
			handle_rpi_input();
		} else
		{ /* error */

		}
	}

	void digital_in_rising_edge() {
		if (this->is_output_ttl) {
			this->output_next_chunk();
		}
	}

	void digital_in_falling_edge() {
	}

	void sampling_timer_interrupt() {
		//if we are currently outputing a chunk, put the next value
		if (currently_outputting_chunk_one == true) {
			if (current_output_one < current_end_chunk_one) {
				this->dac_1->write(*(current_output_one++));
			} else {
				currently_outputting_chunk_one = false;
				if (currently_outputting_chunk_two == false) {
					turn_LED6_off();
				}
			}
		}

		if (currently_outputting_chunk_two == true) {
			if (current_output_two < current_end_chunk_two) {
				this->dac_2->write(*(current_output_two++));
			} else {
				currently_outputting_chunk_two = false;
				if (currently_outputting_chunk_one == false) {
					turn_LED6_off();
				}
			}
		}
	}

public:
	bool is_output_on, is_output_ttl;
	uint32_t buffer_one_size, buffer_two_size;
	float *buffer;
	float *buffer_one, *buffer_two;
	float *current_output_one, *current_output_two;
	float *current_read_one, *current_read_two; //point to the position where the MC is currently reading values from the rpi into the buffer
	uint32_t chunks_one_size, chunks_two_size;
	uint32_t *chunks;
	uint32_t *chunks_one, *chunks_two;
	uint32_t *current_chunk_one, *current_chunk_two;
	float *current_end_chunk_one, *current_end_chunk_two; //points to the end of the current chunk
	uint32_t counter_max, prescaler;
	bool currently_outputting_chunk_one, currently_outputting_chunk_two;
	BasicTimer *sampling_timer;
};


AWGModule *module;

/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

__attribute__((section("sram_func")))
void HAL_GPIO_EXTI_Callback (uint16_t gpio_pin)
{
	if(gpio_pin == DigitalIn_Pin) {
		// Rising Edge
		if(HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_RESET)
			module->digital_in_rising_edge();

		// Falling Edge
		if(HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_SET)
			module->digital_in_falling_edge();
	}

	// Note: Tested with square wave input. Rising and falling edge seem to be inverted?
}

// DMA Interrupts. You probably don't want to change these, they are neccessary for the low-level communications between MCU, converters and RPi
__attribute__((section("sram_func")))
__weak void DMA2_Stream4_IRQHandler(void)
{
	module->dac_2->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream5_IRQHandler(void)
{
	module->dac_1->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream2_IRQHandler(void)
{
	// SPI 1 rx
	module->adc->dma_transmission_callback();
}
__attribute__((section("sram_func")))
void DMA2_Stream3_IRQHandler(void)
{
	// SPI 1 tx - SPI 5 rx
	// no action required
}
__attribute__((section("sram_func")))
void DMA2_Stream0_IRQHandler(void)
{
	module->rpi_dma_in_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream1_IRQHandler(void)
{
	// SPI 4 Tx
	module->rpi->dma_out_interrupt();
}
__attribute__((section("sram_func")))
void DMA2_Stream6_IRQHandler(void)
{
	// SPI 6 Rx
	// no action required
}

__attribute__((section("sram_func")))
void SPI4_IRQHandler(void) {
	module->rpi->spi_interrupt();
}

// This function is called whenever a timer reaches its period
__attribute__((section("sram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2) {
		module->sampling_timer_interrupt();
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
	while (dest < &__sram_func_end) {
	  *dest = *src;
	  src++;
	  dest++;
	}

	/* After power on, give all devices a moment to properly start up */
	HAL_Delay(200);

	module = new AWGModule();

	module->run();

}

#endif
