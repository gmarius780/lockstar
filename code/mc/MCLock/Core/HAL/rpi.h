/*
 * rpi.h
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */

#ifndef HAL_RPI_H_
#define HAL_RPI_H_

#include "../HAL/spi.hpp"
#include "../HAL/dma_new.hpp"

class RPI {
public:
	RPI();
	virtual ~RPI();

	void spi_interrupt();
	void dma_interrupt();

private:
	SPI *spi;
	DMA *dma_in;
	DMA *dma_out;
	bool is_communicating;
	uint32_t current_nbr_of_bytes;
	uint8_t *read_buffer;

	void start_dma_communication(uint32_t nbr_of_bytes);

};

#endif /* HAL_RPI_H_ */
