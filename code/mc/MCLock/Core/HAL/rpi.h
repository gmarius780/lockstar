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


class RPI {
public:
	RPI();
	virtual ~RPI();

	void spi_interrupt();
	void dma_out_interrupt();
	void dma_in_interrupt();

private:
	SPI *spi;
	DMA *dma_in;
	DMA *dma_out;
	DMA_config_t dma_in_config, dma_out_config;

	bool is_communicating; //true after spi interrupt was fired until dma communication is finished
	uint32_t current_nbr_of_bytes;
	uint8_t *read_buffer;
	uint8_t *write_buffer;

	void start_dma_communication(uint32_t nbr_of_bytes);
};

#endif /* HAL_RPI_H_ */

