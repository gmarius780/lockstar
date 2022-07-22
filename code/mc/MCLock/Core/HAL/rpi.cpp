/*
 * rpi.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */

#include "rpi.h"

RPI::RPI() {
	read_buffer = new uint8_t[2550];
	write_buffer = new uint8_t[4096];

	spi = new SPI(4);
	spi->disableSPI();

	/*Configure DMA IN */
	dma_in_config.channel = 4;
	dma_in_config.stream = 0;
	dma_in_config.priority = 1;
	dma_in_config.PAR = (uint32_t)spi->getDRAddress();
	dma_in_config.M0AR = (uint32_t)read_buffer;

	uint8_t DMAprio = 1;
	dma_in_config.priority = DMAprio;
	//important to explicitly disable otherwise the DMA constructor will enable it
	dma_in_config.CR &= ~DMA_SxCR_EN;
	// reset 3 bits that define channel
	dma_in_config.CR &= ~(DMA_SxCR_CHSEL);
	// set channel via 3 control bits
	dma_in_config.CR |= dma_in_config.channel * DMA_SxCR_CHSEL_0;
	// set stream priority from very low (00) to very high (11)
	dma_in_config.CR &= ~(DMA_SxCR_PL);
	// reset 2 bits that define priority
	dma_in_config.CR |= dma_in_config.priority * DMA_SxCR_PL_0; // set priority via 2 control bits
	// increment the memory address with each transfer
	dma_in_config.CR |= DMA_SxCR_MINC;
	// do not increment peripheral address
	dma_in_config.CR &= ~DMA_SxCR_PINC;
	// set direction of transfer to "peripheral to memory"
	dma_in_config.CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
	// Clear DBM bit
	dma_in_config.CR &= (uint32_t)(~DMA_SxCR_DBM);
	// disable transmission-complete / halftransmission; enable error interrupt
	dma_in_config.CR  &= ~DMA_SxCR_TCIE;
	dma_in_config.CR  &= ~DMA_SxCR_HTIE;
	dma_in_config.CR  |= DMA_SxCR_TEIE;

	dma_in = new DMA(dma_in_config);
//	dma_in->disableCircMode();


	/*configure DMA out*/
	dma_out_config.stream     = 1;
	dma_out_config.channel    = 4;
	dma_out_config.PAR        = (uint32_t)spi->getDRAddress();
	dma_out_config.M0AR       = (uint32_t)write_buffer;
	dma_out_config.NDTR       = 0;
	dma_out_config.priority = 1;

	//important to explicitly disable otherwise the DMA constructor will enable it
	dma_out_config.CR &= ~DMA_SxCR_EN;
	dma_out_config.CR &= ~(DMA_SxCR_CHSEL); // reset 3 bits that define channel
	dma_out_config.CR |= dma_out_config.channel * DMA_SxCR_CHSEL_0; // set channel via 3 control bits
	// set stream priority from very low (00) to very high (11)
	dma_out_config.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
	dma_out_config.CR |= dma_out_config.priority * DMA_SxCR_PL_0; // set priority via 2 control bits
	// increment the memory address with each transfer
	dma_out_config.CR |= DMA_SxCR_MINC;
	// do not increment peripheral address
	dma_out_config.CR &= ~DMA_SxCR_PINC;
	// set direction of transfer to "memory to peripheral"
	dma_out_config.CR &= ~DMA_SxCR_DIR_1;
	dma_out_config.CR |= DMA_SxCR_DIR_0;
	// Clear DBM bit
	dma_out_config.CR &= (uint32_t)(~DMA_SxCR_DBM);
	// Program transmission-complete interrupt
	dma_out_config.CR  &= ~DMA_SxCR_TCIE;
	dma_out_config.CR  &= ~DMA_SxCR_TEIE;

	dma_out = new DMA(dma_out_config);

	spi->bindDMAHandlers(dma_out, dma_in);

	is_communicating = false;
	spi->enableRxIRQ();
	spi->enableSPI();

}

RPI::~RPI() {
	// TODO Auto-generated destructor stub
}

void RPI::spi_interrupt() {
	if(is_communicating == false) {
		is_communicating = true;
		current_nbr_of_bytes = 10 * ((uint32_t)*(volatile uint8_t *)spi->getDRAddress());
		this->start_dma_communication(this->current_nbr_of_bytes);
	}
}

void RPI::dma_in_interrupt() {
	// wait while SPI is busy

	while(spi->SPI_regs->SR & SPI_SR_BSY);
	spi->disableSPI_DMA();
	dma_in->disableDMA();
	dma_in->resetTransferCompleteInterruptFlag();
	is_communicating = false;
	//clear interrupt
}

void RPI::dma_out_interrupt() {
	spi->disableSPI_DMA();
	dma_out->resetTransferCompleteInterruptFlag();
}

void RPI::start_dma_communication(uint32_t nbr_of_bytes) {
	spi->disableRxIRQ();
	dma_in->enable_tc_irq();

	this->dma_in->setNumberOfData(nbr_of_bytes);
	this->dma_in->enableDMA();
	spi->enableSPI_DMA();
	//spi->enableSPI();
}

