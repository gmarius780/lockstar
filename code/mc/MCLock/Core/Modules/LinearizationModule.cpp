/*
 * AWGModule.cpp
 *
 *
 *  Created on: Oct 31, 2022
 *      Author: Samuel
 */

#include "LinearizationModule.h"

LinearizationModule *module;

LinearizationModule::LinearizationModule() {
	initialize_rpi();
	turn_LED6_off();
	turn_LED5_on();

	timer_arr = 0;
	timer_psc = 0;

	ramp_pointer = 0;
	toggle = true;

	timer = new BasicTimer(2,timer_arr,timer_psc,false);

	reset_state_machine();
	ready_to_work = true;
	received_inverted_pivots = false;
	output_range = 0;
	output_min = 0;
	output_max = 0;
	ramp_range = 0;
	pivot_spacing = 0;
	number_of_ramp_packages = 0;
	number_of_received_ramp_values = 0;
	ramp_package_counter = 0;
	test = false;
}

void LinearizationModule::LinearizationModule::run() {
	initialize_adc(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
	initialize_dac();
	this->dac_1->write(0);
	this->dac_2->write(0);
	float output = 0;

	/*** work loop ***/
	while(!test) {
		if(!ready_to_work) { new_linearization(); }
		dac_1->write(linearize_output(output));
	}
	timer->enable_interrupt();
	timer->enable();
	while(true);
}

float LinearizationModule::linearize_output(float target_output) {
	if(!received_inverted_pivots)
		return target_output;

	target_output = target_output/ramp_range*output_range;

	uint32_t pivot_index = (uint32_t)((target_output-output_min)/output_range*ramp_length);
	float pivot = output_min + pivot_spacing*pivot_index;
	float interpolation = (target_output-pivot)/pivot_spacing;
	if(interpolation < 0)
		interpolation = 0;
	if(pivot_index > ramp_length-2)
		return inverted_pivots_buffer[pivot_index];
	return inverted_pivots_buffer[pivot_index] + (inverted_pivots_buffer[pivot_index+1]-inverted_pivots_buffer[pivot_index])*interpolation;
}

void LinearizationModule::new_linearization() {
	RPIDataPackage* ack = rpi->get_write_package();
	ack->push_ack();
	rpi->send_package(ack);

	while(!timer_initialized);
	while(!received_new_ramp);
	while(!measurement_trigger);
	gain_measurement();
	while(!finished_gain_measurement);
	while(!response_measurement_sent_to_rpi);

	reset_state_machine();
	ready_to_work = true;
}

void LinearizationModule::reset_state_machine() {
	received_new_ramp = false;
	timer_initialized = false;
	measurement_trigger = false;
	finished_gain_measurement = false;
	response_measurement_sent_to_rpi = false;
	number_of_ramp_packages = 0;
	number_of_received_ramp_values = 0;
	ramp_package_counter = 0;
}

void LinearizationModule::handle_rpi_input() {

	/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/

	RPIDataPackage* read_package = this->rpi->get_read_package();
	//switch between method_identifier
	switch (read_package->pop_from_buffer<uint32_t>()) {
		case NEW_LINEARIZATION:
			ready_to_work = false;
			break;

		// Initialize timer
		case INITIALIZE_TIMER:
			initialize_timer(read_package);
			break;
		// Initialize ramp buffer
		case INITIALIZE_NEW_RAMP:
			initialize_new_ramp(read_package);
			break;

		// Write ramp buffer
		case SET_RAMP:
			set_ramp(read_package);
			break;

		// Start measurement
		case TRIGGER_GAIN_MEASUREMENT:
			trigger_gain_measurement();
			break;

		case SEND_GAIN_MEASUREMENT:
			send_gain_measurement(read_package);
			break;

		case SET_INVERTED_PIVOTS:
			set_inverted_pivots(read_package);
			break;

		default: {
			/*** send NACK because the method_identifier is not valid ***/
			RPIDataPackage* write_package = rpi->get_write_package();
			write_package->push_nack();
			rpi->send_package(write_package);
			break;
		}
	}

}

/*** START: METHODS ACCESSIBLE FROM THE RPI ***/

void LinearizationModule::initialize_timer(RPIDataPackage* read_package) {
	//turn_LED5_off();
	timer_arr = read_package->pop_from_buffer<uint32_t>();
	timer_psc = read_package->pop_from_buffer<uint32_t>();

	timer->set_auto_reload(timer_arr);
	timer->set_prescaler(timer_psc);
	timer->enable_interrupt();
	timer_initialized = true;

	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}

void LinearizationModule::initialize_new_ramp(RPIDataPackage* read_package) {
	ramp_length = read_package->pop_from_buffer<uint32_t>();
	ramp_buffer = new float[ramp_length]();
	inverted_pivots_buffer = new float[ramp_length]();
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);
}

void LinearizationModule::set_ramp(RPIDataPackage* read_package) {

	if(ramp_package_counter == 0) {
		number_of_ramp_packages = read_package->pop_from_buffer<uint32_t>();
		ramp_package_sizes = new uint32_t[number_of_ramp_packages]();
	} else {
		uint32_t current_package_size = read_package->pop_from_buffer<uint32_t>();
		for(uint32_t i=0; i<current_package_size; i++) {
			ramp_buffer[number_of_received_ramp_values + i] = read_package->pop_from_buffer<float>();
		}
		number_of_received_ramp_values += current_package_size;
		ramp_package_sizes[ramp_package_counter-1] = current_package_size;
	}

	if(ramp_package_counter == number_of_ramp_packages) {
		RPIDataPackage* ack = rpi->get_write_package();
		ack->push_ack();
		rpi->send_package(ack);
		ramp_package_counter = 0;
		number_of_received_ramp_values = 0;
		received_new_ramp = true;
		return;
	}
	ramp_package_counter++;
}

void LinearizationModule::trigger_gain_measurement() {
	measurement_trigger = true;
}

void LinearizationModule::gain_measurement() {
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);

	timer->enable();

	// Wait for measurement to finish
	// (+3 to be safe, included if(ramp_pointer < ...) in TIM interrupt)
	while(ramp_pointer < ramp_length+3);

	timer->disable();
	timer->disable_interrupt();

	ramp_range = ramp_buffer[ramp_length-1] - ramp_buffer[0];
	output_min = inverted_pivots_buffer[0];
	output_max = inverted_pivots_buffer[ramp_length-1];
	output_range = output_max - output_min;
	pivot_spacing = output_range/ramp_length;

	ramp_pointer = 0;
	// Measurement is stored in inverted_pivots_buffer to save some memory
	finished_gain_measurement = true;
}

void LinearizationModule::send_gain_measurement(RPIDataPackage* read_package) {
	RPIDataPackage* write_package = rpi->get_write_package();
	if(!finished_gain_measurement) {
		write_package->push_nack();
		rpi->send_package(write_package);
		return;
	}

	uint32_t number_of_sent_ramp_values = 0;
	for(uint32_t package_number=0; package_number<number_of_ramp_packages; package_number++) {
		RPIDataPackage* data_package = rpi->get_write_package();
		for(uint32_t i=0; i<ramp_package_sizes[package_number]; i++) {
			data_package->push_to_buffer<float>(inverted_pivots_buffer[number_of_sent_ramp_values + i]);
		}
		number_of_sent_ramp_values += ramp_package_sizes[package_number];
		rpi->send_package(data_package);
	}

	response_measurement_sent_to_rpi = true;
}

void LinearizationModule::set_inverted_pivots(RPIDataPackage* read_package) {
	for(uint32_t i=0; i<ramp_package_sizes[ramp_package_counter]; i++) {
		inverted_pivots_buffer[number_of_received_ramp_values+i] = read_package->pop_from_buffer<float>();
	}

	if(ramp_package_counter == number_of_ramp_packages) {
		received_inverted_pivots = true;

		RPIDataPackage* write_package = rpi->get_write_package();
		write_package->push_ack();
		rpi->send_package(write_package);
	}
	ramp_package_counter++;
}


/*** END: METHODS ACCESSIBLE FROM THE RPI ***/


void LinearizationModule::rpi_dma_in_interrupt() {

	if(rpi->dma_in_interrupt())
	{ /*got new package from rpi*/
		handle_rpi_input();
	} else
	{ /* error */

	}
}



/******************************
 *         INTERRUPTS          *
 ******************************
 * Callbacks are functions that are executed in response to events such as SPI communication finished, change on trigger line etc */

__attribute__((section("sram_func")))
void HAL_GPIO_EXTI_Callback (uint16_t gpio_pin)
{
	/*if(gpio_pin == DigitalIn_Pin) {
		// Rising Edge
		if(HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_RESET)
			//module->digital_in_rising_edge();

		// Falling Edge
		if(HAL_GPIO_ReadPin(DigitalIn_GPIO_Port, DigitalIn_Pin) == GPIO_PIN_SET)
			//module->digital_in_falling_edge();
	}*/

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
// !!!!!! GOT HARD FAULT WITH THIS ATTRIBUTE SET, BUT ONLY ON LOCKSTAR IN D15.......??
//__attribute__((section("sram_func")))
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2) {
		if(module->test) {
			if(module->ramp_pointer > module->ramp_length-1)
				module->ramp_pointer = 0;
			float t = module->linearize_output(module->ramp_buffer[module->ramp_pointer]);
			module->dac_1->write(t);
			module->ramp_pointer++;
		} else {
			if(module->ramp_pointer < module->ramp_length)
				module->dac_1->write(module->ramp_buffer[module->ramp_pointer]);

			if(module->ramp_pointer > 0 && module->ramp_pointer < module->ramp_length+1)
				module->adc->start_conversion();

			if(module->ramp_pointer > 1 && module->ramp_pointer < module->ramp_length+2)
				module->inverted_pivots_buffer[module->ramp_pointer-2] = module->adc->channel2->get_result();

			module->ramp_pointer++;
		}

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

	module = new LinearizationModule();

	module->run();

}

