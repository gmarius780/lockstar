/*
 * rpi.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */

#include "rpi.h"
/*
RPI::RPI() {
	read_buffer = new uint8_t[2550];
	write_buffer = new uint8_t[4096];

	this->spi = SPI(4);
	DMA_config_struct dma_in_config;
	dma_in_config.channel = 4;
	dma_in_config.stream = 0;
	dma_in_config.priority = 1;
	dma_in_config.PAR = this->spi->getDRAddress();
	dma_in_config.M0AR = this->read_buffer;

	uint8_t DMAprio = 1;
	dma_in_config.priority = DMAprio;
	// reset 3 bits that define channel
	dma_in_config.CR &= ~(DMA_SxCR_CHSEL);
	// set channel via 3 control bits
	dma_in_config.CR |= 4 * DMA_SxCR_CHSEL_0;
	// set stream priority from very low (00) to very high (11)
	dma_in_config.CR &= ~(DMA_SxCR_PL);
	// reset 2 bits that define priority
	dma_in_config.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
	// increment the memory address with each transfer
	dma_in_config.CR |= DMA_SxCR_MINC;
	// do not increment peripheral address
	dma_in_config.CR &= ~DMA_SxCR_PINC;
	// set direction of transfer to "peripheral to memory"
	dma_in_config.CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
	// Clear DBM bit
	dma_in_config.CR &= (uint32_t)(~DMA_SxCR_DBM);
	// Program transmission-complete interrupt
	dma_in_config.CR  |= DMA_SxCR_TCIE;


	this->dma_in = new DMA(dma_in_config);
	this->dma_in->disableCircMode();
	is_communicating = false;

}

RPI::~RPI() {
	// TODO Auto-generated destructor stub
}

void RPI::spi_interrupt() {
	if(is_communicating == false) {
		is_communicating = true;
		this->current_nbr_of_bytes = 10 * ((uint32_t)*(volatile uint8_t *)spi->getDRAddress());

		this->start_dma_communication(this->current_nbr_of_bytes);
	}
}

void RPI::dma_interrupt() {
	is_communicating = false;
	//clear interrupt
}

void RPI::start_dma_communication(uint32_t nbr_of_bytes) {
	this->dma_in->setNumberOfData(nbr_of_bytes);
	this->dma_in->enableDMA();
}
*/
