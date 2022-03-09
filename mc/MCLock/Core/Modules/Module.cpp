/*
 * Module.cpp
 *
 *  Created on: Feb 25, 2022
 *      Author: qo
 */

#include "Module.hpp"

Module::Module() {
	this->module_state = module_state_rpi_init_pending;

}

Module::~Module() {
	// TODO Auto-generated destructor stub
}

/**
 * Interrupts change the modules corresponding states
 */

void Module::rpi_interrupt() {
	this->rpi_input_state = rpi_input_pending;
}

void Module::run() {
	this->init();
	this->loop();
}


void Module::handle_rpi_input() {
	//read data, decide what to do, create RpiData object?!
	//if rpi_initialize is called --> change module state to module_state_running
}

void Module::timer_interrupt() {

}

void Module::trigger_interrupt() {

}

void Module::adc_interrupt() {

}

void Module::rpi_init() {
	// read init params
	/* Set up DAC output range */
	this->DAC_1->ConfigOutputs(&hadc3, ADC_CHANNEL_14, ADC_CHANNEL_9);
	//DAC_1->Setup(DAC_BIPOLAR_10V, false);
	//while(!DAC_1->isReady());
	this->DAC_2->ConfigOutputs(&hadc3, ADC_CHANNEL_8, ADC_CHANNEL_15);
	//DAC_2->Setup(DAC_BIPOLAR_10V, false);
	//while(!DAC_2->isReady());
	this->DAC_1->WriteFloat(0.0);
	this->DAC_2->WriteFloat(0.0);
	while(!this->DAC_1->isReady() && !this->DAC_2->isReady());

	// set the "power on" indicator LED
	turn_LED5_on();
	this->module_state = module_state_rpi_init_pending;
}

/**
 * Initialize peripheral devices
 *
 */
void Module::init() {
	/* Set up input and output devices
		 * The devices communicate with the microcontroller via SPI (serial peripheral interace). For full-speed operation,
	 * a DMA controller (direct memory access) is made responsible for shifting data between SPI registers and RAM on
	 * the microcontroller. See the STM32F4xx reference for more information, in particular for the DMA streams/channels
	 * table.
	 */
	this->ADC_DEV = new ADC_Dev(/*SPI number*/ 1, /*DMA Stream In*/ 2, /*DMA Channel In*/ 3, /*DMA Stream Out*/ 3, /*DMA Channel Out*/ 3,
			/* conversion pin port*/ ADC_CNV_GPIO_Port, /* conversion pin number*/ ADC_CNV_Pin);
	//ADC_DEV->Channel1->Setup(ADC_OFF);
	this->DAC_1 = new DAC_Dev(/*SPI number*/ 6, /*DMA Stream Out*/ 5, /*DMA Channel Out*/ 1, /*DMA Stream In*/ 6, /*DMA Channel In*/ 1, /* sync pin port*/ DAC_1_Sync_GPIO_Port,
			/* sync pin number*/ DAC_1_Sync_Pin, /* clear pin port*/ CLR6_GPIO_Port, /* clear pin number*/ CLR6_Pin);
	this->DAC_2 = new DAC_Dev(/*SPI number*/ 5, /*DMA Stream Out*/ 4, /*DMA Channel Out*/ 2, /*DMA Stream In*/ 3, /*DMA Channel In*/ 2, /* sync pin port*/ DAC_2_Sync_GPIO_Port,
			/* sync pin number*/ DAC_2_Sync_Pin, /* clear pin port*/ CLR5_GPIO_Port, /* clear pin number*/ CLR5_Pin);
	this->RPi = new RaspberryPi(/*SPI number*/ 4, /*DMA Stream In*/ 0, /*DMA Channel In*/ 4, /*DMA Stream Out*/ 1,
			/*DMA Channel Out*/ 4, /*GPIO Port*/ Pi_Int_GPIO_Port, /*GPIO Pin*/ Pi_Int_Pin);

	DigitalOutLow();
}

void Module::work() {

}

void Module::loop() {

	while (this->module_state == module_state_rpi_init_pending) {
		HAL_Delay(1000);
		if (this->rpi_input_state == rpi_input_pending) {
			this->handle_rpi_input();
		}
	}

	while (1) {
		this->work();
		if (this->rpi_input_state == rpi_input_pending) {
			this->handle_rpi_input();
		}
	}
}
