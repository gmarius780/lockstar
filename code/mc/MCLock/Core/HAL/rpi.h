/*
 * rpi.h
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */

#ifndef HAL_RPI_H_
#define HAL_RPI_H_
#include "stm32f427xx.h"
#include "../HAL/dma_new.hpp"
#include "../HAL/spi.hpp"
#include "../Lib/RPIDataPackage.h"
#include "../HAL/BasicTimer.hpp"


class RPI {
public:
	RPI();
	virtual ~RPI();

	void spi_interrupt();
	void dma_out_interrupt();
	void dma_out_error_interrupt();
	void comm_reset_timer_interrupt();
	bool dma_in_interrupt(); //true if communication was successful
	void dma_in_error_interrupt();

	RPIDataPackage* get_read_package();
	RPIDataPackage* get_write_package();

	void send_package(RPIDataPackage* write_package);
	volatile uint8_t* get_read_buffer();
private:
	SPI *spi;
	DMA *dma_in;
	DMA *dma_out;
	DMA_config_t dma_in_config, dma_out_config;

	BasicTimer *comm_reset_timer; // resets is_communicating after an one periode of this timer
	//COMMUNICATION RESET FREQUENCY= INTERNAL_CLOCK_FREQUENCY/prescaler * counter_max = 90e6/90'00*10000 = 1Hz
	static const uint32_t COMM_RESET_COUNTER_MAX = 10000;
	static const uint32_t COMM_RESET_PRESCALER = 9000;

	bool is_communicating; //true after spi interrupt was fired until dma communication is finished
	uint32_t current_nbr_of_bytes;
	volatile uint8_t *read_buffer;
	volatile uint8_t *write_buffer;

	void start_dma_in_communication(uint32_t nbr_of_bytes);
	void start_dma_out_communication(uint32_t nbr_of_bytes);


};

#endif /* HAL_RPI_H_ */

