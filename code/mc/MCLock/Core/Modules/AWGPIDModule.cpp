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


#include "../HAL/leds.hpp"
#include "../Lib/pid.hpp"

#include "BufferBaseModule.h"

#ifdef AWG_PID_MODULE


/**
 * User can upload buffers containing setpoints for both channels. The module will then be a pid controller for both outputs and use the buffer as setpoints with a
 * sampling rate, set by the user
 */
class AWGPIDModule: public BufferBaseModule {
	static const uint32_t BUFFER_LIMIT_kBYTES = 160; //if this is chosen to large (200) there is no warning, the MC simply crashes (hangs in syscalls.c _exit())
	static const uint32_t MAX_NBR_OF_CHUNKS = 100;
public:
	AWGPIDModule() {
		initialize_rpi();
		turn_LED6_off();
		turn_LED5_on();
		//default sampling rate is INTERNAL_CLOCK_FREQUENCY/prescaler * counter_max = 90e6/90*1000 = 1khz
		prescaler=90;
		counter_max=1000;
		this->sampling_timer = new BasicTimer(2, counter_max, prescaler);

		// allocate buffer and chunk space
		this->buffer = new float[BUFFER_LIMIT_kBYTES*250]; //contains buffer_one and buffer_two sequentially
		this->chunks = new uint32_t[MAX_NBR_OF_CHUNKS]; //contains chuncks_one and chunks_two sequentially

		this->pid_one = new PID(0., 0., 0., 0., 0.);
		this->pid_two = new PID(0., 0., 0., 0., 0.);

		this->setpoint_one = this->setpoint_two = 0.;
		this->locked = false;
	}

	__attribute__((optimize(0)))
	void run() {
		initialize_adc_dac(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
		this->dac_1->write(0);
		this->dac_2->write(0);

		//timer to measure elapsed time
		const uint16_t psc = 68;
		const float TIM3freq = 90e6;
		BasicTimer* timer = new BasicTimer(3, 0xFFFF, psc);
		timer->enable();

		float dt = 0;
		uint16_t t = 0;

		/*** work loop ***/
		while(true) {
			while(this->locked and (is_output_on or is_output_ttl)) {
				// Measuring elapsed time per work loop
				t = timer->get_counter() - t;
				dt = t/TIM3freq*psc;
				t = timer->get_counter();

				this->dac_1->write(this->pid_one->calculate_output(this->setpoint_one, adc->channel1->get_result(), dt));
				this->dac_2->write(this->pid_two->calculate_output(this->setpoint_two, adc->channel2->get_result(), dt));
			}


		}
	}

	void handle_rpi_input() {
		if(BufferBaseModule::handle_rpi_base_methods() == false) { //if base class doesn't know the called method
			/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/
			RPIDataPackage* read_package = this->rpi->get_read_package();
			//switch between method_identifier
			switch (read_package->pop_from_buffer<uint32_t>()) {
			case METHOD_SET_PID_ONE:
				set_pid_one(read_package);
				break;
			case METHOD_SET_PID_TWO:
				set_pid_two(read_package);
				break;
			case METHOD_LOCK:
				lock(read_package);
				break;
			case METHOD_UNLOCK:
				unlock(read_package);
				break;
			case METHOD_ENABLE_INTENSITY_LOCK_MODE_ONE:
				enable_intensity_lock_mode_one(read_package);
				break;
			case METHOD_DISABLE_INTENSITY_LOCK_MODE_ONE:
				disable_intensity_lock_mode_one(read_package);
				break;
			case METHOD_ENABLE_INTENSITY_LOCK_MODE_TWO:
				enable_intensity_lock_mode_two(read_package);
				break;
			case METHOD_DISABLE_INTENSITY_LOCK_MODE_TWO:
				disable_intensity_lock_mode_two(read_package);
				break;
			default:
					/*** send NACK because the method_identifier is not valid ***/
					RPIDataPackage* write_package = rpi->get_write_package();
					write_package->push_nack();
					rpi->send_package(write_package);
					break;
			}

		}
	}

	/*** START: METHODS ACCESSIBLE FROM THE RPI ***/
	static const uint32_t METHOD_SET_PID_ONE = 31;
	void set_pid_one(RPIDataPackage* read_package) {
		/***Read arguments***/
		float p = read_package->pop_from_buffer<float>();
		float i = read_package->pop_from_buffer<float>();
		float d = read_package->pop_from_buffer<float>();
		float input_offset = read_package->pop_from_buffer<float>();
		float output_offset = read_package->pop_from_buffer<float>();
		float i_threshold = read_package->pop_from_buffer<float>();

		this->pid_one->set_pid(p, i, d, input_offset, output_offset, i_threshold);

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_SET_PID_TWO = 32;
	void set_pid_two(RPIDataPackage* read_package) {
		/***Read arguments***/
		float p = read_package->pop_from_buffer<float>();
		float i = read_package->pop_from_buffer<float>();
		float d = read_package->pop_from_buffer<float>();
		float input_offset = read_package->pop_from_buffer<float>();
		float output_offset = read_package->pop_from_buffer<float>();
		float i_threshold = read_package->pop_from_buffer<float>();

		this->pid_two->set_pid(p, i, d, input_offset, output_offset, i_threshold);

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_LOCK = 33;
	void lock(RPIDataPackage* read_package) {
		this->locked = true;
		turn_LED6_on();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_UNLOCK = 34;
	void unlock(RPIDataPackage* read_package) {
		this->locked = false;
		turn_LED6_off();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_ENABLE_INTENSITY_LOCK_MODE_ONE = 35;
	void enable_intensity_lock_mode_one(RPIDataPackage* read_package) {
		this->pid_one->enable_intensity_mode();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_DISABLE_INTENSITY_LOCK_MODE_ONE = 36;
	void disable_intensity_lock_mode_one(RPIDataPackage* read_package) {
		this->pid_one->disable_intensity_mode();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_ENABLE_INTENSITY_LOCK_MODE_TWO = 37;
	void enable_intensity_lock_mode_two(RPIDataPackage* read_package) {
		this->pid_two->enable_intensity_mode();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}

	static const uint32_t METHOD_DISABLE_INTENSITY_LOCK_MODE_TWO = 38;
	void disable_intensity_lock_mode_two(RPIDataPackage* read_package) {
		this->pid_two->disable_intensity_mode();

		/*** send ACK ***/
		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}
	/*** END: METHODS ACCESSIBLE FROM THE RPI ***/


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
			//this->enable_sampling();
		}
	}

	void digital_in_falling_edge() {
	}

	//__attribute__((section("sram_func")))
	void sampling_timer_interrupt() {
		if (current_output_one < current_end_chunk_one) {
			this->setpoint_one = *(current_output_one++);
		} else {
			if (current_output_two >= current_end_chunk_two) {
				turn_LED6_off();
				this->disable_sampling();
			}
		}

		if (current_output_two < current_end_chunk_two) {
			this->setpoint_two = *(current_output_two++);
		} else {
			if (current_output_one >= current_end_chunk_one) {
				turn_LED6_off();
				this->disable_sampling();
			}
		}
	}

public:
	PID* pid_one;
	PID* pid_two;
	float setpoint_one, setpoint_two;
	bool locked;
};


AWGPIDModule *module;

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
	else if (htim->Instance == TIM4) {
		module->rpi->comm_reset_timer_interrupt();
	}
	else if(htim->Instance == TIM7) {
		module->scope_timer_interrupt();
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

	module = new AWGPIDModule();

	module->run();

}

#endif
