/*
 * AWGModule.cpp
 *
 *
 *  Created on: Oct 31, 2022
 *      Author: Samuel
 */


#include "LinearizationModule.h"
#ifdef LINEARIZATION_MODULE

LinearizationModule *module;

LinearizationModule::LinearizationModule() {
	initialize_rpi();
	turn_LED6_off();
	turn_LED5_on();

	timer_arr = 0;
	timer_psc = 0;

	buffer_pointer = 0;

	timer = new BasicTimer(2,timer_arr,timer_psc);

	reset_state_machine();
	ready_to_work = true;
	linearization_active = false;

	ramp_start = 0;
	ramp_end = 0;
	ramp_stepsize = 0;
	output_range = 0;
	output_min = 0;
	output_max = 0;
	ramp_range = 0;
	pivot_spacing = 0;
	test = false;
}

void LinearizationModule::LinearizationModule::run() {
	initialize_adc(ADC_UNIPOLAR_10V, ADC_UNIPOLAR_10V);
	initialize_dac();
	this->dac_1->write(0);
	this->dac_2->write(0);
	float output = 0;

	bool toggle = false;

	/*** work loop ***/
	while(!test) {
		if(!ready_to_work) { new_linearization(); }

		if(toggle) {
			dac_1->write(0);
		} else {
			dac_1->write(3);
		}

		toggle = !toggle;
		toggle = !toggle;
		toggle = !toggle;
	}
	timer->set_auto_reload(200);
	timer->enable_interrupt();
	timer->enable();
	while(true);
}

float LinearizationModule::linearize_output(float target_output) {
	if(!linearization_active)
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

void output_ramp() {
	if(module->buffer_pointer < module->ramp_length)
		module->dac_1->write(module->ramp_start + module->ramp_stepsize*module->buffer_pointer);

	if(module->buffer_pointer > 0 && module->buffer_pointer < module->ramp_length+1)
		module->adc->start_conversion();

	if(module->buffer_pointer > 1 && module->buffer_pointer < module->ramp_length+2)
		module->inverted_pivots_buffer[module->buffer_pointer-2] = module->adc->channel2->get_result();

	module->buffer_pointer++;
}

void LinearizationModule::new_linearization() {
	RPIDataPackage* ack = rpi->get_write_package();
	ack->push_ack();
	rpi->send_package(ack);

	perform_gain_measurement();
	while(!finished_gain_measurement);
	ack = rpi->get_write_package();
	ack->push_ack();
	rpi->send_package(ack);

	while(!sent_gain_measurement);
	while(!received_inverted_pivots);

	reset_state_machine();

	ready_to_work = true;
}

void LinearizationModule::reset_state_machine() {
	received_ramp_paramters = false;
	finished_gain_measurement = false;
	sent_gain_measurement = false;
	received_inverted_pivots = false;
}

void LinearizationModule::handle_rpi_input() {

	/*** Package format: method_identifier (uint32_t) | method specific arguments (defined in the methods directly) ***/

	RPIDataPackage* read_package = this->rpi->get_read_package();
	//switch between method_identifier
	switch (read_package->pop_from_buffer<uint32_t>()) {
		case SET_RAMP_PARAMETERS:
			set_ramp_parameters(read_package);
			ready_to_work = false;
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

void LinearizationModule::set_ramp_parameters(RPIDataPackage* read_package) {
	ramp_start = read_package->pop_from_buffer<float>();
	ramp_end = read_package->pop_from_buffer<float>();
	ramp_length = read_package->pop_from_buffer<uint32_t>();
	ramp_range = ramp_end - ramp_start;
	ramp_stepsize = ramp_range/ramp_length;

	inverted_pivots_buffer = new float[ramp_length]();

	timer_arr = read_package->pop_from_buffer<uint32_t>();
	timer_psc = read_package->pop_from_buffer<uint32_t>();

	timer->set_auto_reload(timer_arr);
	timer->set_prescaler(timer_psc);
	timer->enable_interrupt();
}

void LinearizationModule::perform_gain_measurement() {
	RPIDataPackage* write_package = rpi->get_write_package();
	write_package->push_ack();
	rpi->send_package(write_package);

	timer->enable();

	// Wait for measurement to finish
	// (+3 to be safe, included if(ramp_pointer < ...) in TIM interrupt)
	while(buffer_pointer < ramp_length+3);

	timer->disable();
	timer->disable_interrupt();

	output_min = inverted_pivots_buffer[0];
	output_max = inverted_pivots_buffer[ramp_length-1];
	output_range = output_max - output_min;
	pivot_spacing = output_range/ramp_length;

	buffer_pointer = 0;
	// Measurement is stored in inverted_pivots_buffer to save some memory
	finished_gain_measurement = true;
}

void LinearizationModule::send_gain_measurement(RPIDataPackage* read_package) {
	uint32_t buffer_offset = read_package->pop_from_buffer<uint32_t>();
	uint32_t package_size = read_package->pop_from_buffer<uint32_t>();
	bool last_package = read_package->pop_from_buffer<bool>();

	if(buffer_offset+package_size > ramp_length) {
		RPIDataPackage* nack = rpi->get_write_package();
		nack->push_nack();
		rpi->send_package(nack);
		return;
	}

	RPIDataPackage* data_package = rpi->get_write_package();
	for(uint32_t i=0; i<package_size; i++)
		data_package->push_to_buffer<float>(inverted_pivots_buffer[buffer_offset + i]);
	rpi->send_package(data_package);

	if(last_package)
		sent_gain_measurement = true;
}


void LinearizationModule::set_inverted_pivots(RPIDataPackage* read_package) {
	uint32_t buffer_offset = read_package->pop_from_buffer<uint32_t>();
	uint32_t package_size = read_package->pop_from_buffer<uint32_t>();
	bool last_package = read_package->pop_from_buffer<bool>();

	if(buffer_offset+package_size > ramp_length) {
		RPIDataPackage* nack = rpi->get_write_package();
		nack->push_nack();
		rpi->send_package(nack);
	}

	for(uint32_t i=0; i<package_size; i++) {
		inverted_pivots_buffer[buffer_offset+i] = read_package->pop_from_buffer<float>();
	}

	RPIDataPackage* ack = rpi->get_write_package();
	ack->push_ack();
	rpi->send_package(ack);

	if(last_package) {
		received_inverted_pivots = true;
		linearization_active = true;
	}

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
			if(module->buffer_pointer > module->ramp_length-1)
				module->buffer_pointer = 0;
			float t = module->linearize_output(module->ramp_start + module->ramp_stepsize*module->buffer_pointer);
			module->dac_1->write(t);
			module->buffer_pointer++;
		} else { // add output_ramp state
			output_ramp();
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

#endif

