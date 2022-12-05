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

	/*Configure communication_reset_timer
	 * It is used to reset is_comminicating, in case the communication failed
	 * */
	comm_reset_timer = new BasicTimer(4, COMM_RESET_COUNTER_MAX, COMM_RESET_PRESCALER);
	comm_reset_timer->disable();
	comm_reset_timer->disable_interrupt();
	comm_reset_timer->reset_counter();

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
	//initialize CR to be zero
	dma_in_config.CR = 0;
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

	//disable peripheral flow control -> the MC knows how many bytes to expect (the rpi tells it via spi)
	dma_in_config.CR &= ~DMA_SxCR_PFCTRL;

	//set memory 'unit' to 8-bit
	dma_in_config.CR &= ~DMA_SxCR_PSIZE_0;
	dma_in_config.CR &= ~DMA_SxCR_PSIZE_1;
	dma_in_config.CR &= ~DMA_SxCR_MSIZE_0;
	dma_in_config.CR &= ~DMA_SxCR_MSIZE_1;


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

	//initialize CR to be zero
	dma_out_config.CR = 0;

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
	//disable peripheral flow control -> the MC knows how many bytes to expect (the rpi tells it via spi)
	dma_out_config.CR &= ~DMA_SxCR_PFCTRL;

	dma_out = new DMA(dma_out_config);

	is_communicating = false;
	while(spi->isBusy());
	spi->enableRxIRQ();
	spi->disableTxIRQ();
	spi->enableSPI();

}

RPI::~RPI() {
	// TODO Auto-generated destructor stub
}

void RPI::spi_interrupt() {
	if(is_communicating == false) {
		//get new command from rpi
		comm_reset_timer->enable_interrupt();
		comm_reset_timer->enable();
		current_nbr_of_bytes = 10 * ((uint32_t)*(volatile uint8_t *)spi->getDRAddress());
		if (current_nbr_of_bytes != 0) {
			is_communicating = true;
			this->start_dma_in_communication(this->current_nbr_of_bytes);
		}
	}
}

bool RPI::dma_in_interrupt() {
	this->comm_reset_timer->disable();
	this->comm_reset_timer->disable_interrupt();
	this->comm_reset_timer->reset_counter();
	if (dma_in->transfer_complete()) {

		if (dma_in->getNumberOfData() <= 0) {
			while(spi->isBusy());
			spi->disableSPI_DMA();
			dma_in->disableDMA();
			dma_in->resetTransferCompleteInterruptFlag();
			is_communicating = false;
			spi->enableRxIRQ();
			return true;
		} else {
			return false;
		}
	} else {
		dma_in_error_interrupt();
		return false;
	}
}

void RPI::comm_reset_timer_interrupt() {
	if (this->is_communicating == true) {
		// reset rpi-communication
		this->is_communicating = false;
		this->current_nbr_of_bytes = 0;
		while(spi->isBusy());
		spi->disableSPI_DMA();
		dma_in->disableDMA();
		dma_in->resetTransferCompleteInterruptFlag();
		is_communicating = false;
		spi->enableRxIRQ();
		this->comm_reset_timer->disable();
		this->comm_reset_timer->disable_interrupt();
		this->comm_reset_timer->reset_counter();
	}
}

void RPI::dma_in_error_interrupt() {
}

void RPI::dma_out_interrupt() {
	if (dma_out->transfer_complete()) {
		while(spi->isBusy());
		spi->disableSPI_DMA();
		dma_out->disableDMA();
		dma_out->resetTransferCompleteInterruptFlag();
	} else {
		dma_out_error_interrupt();
	}
}

void RPI::dma_out_error_interrupt() {

}

void RPI::start_dma_in_communication(uint32_t nbr_of_bytes) {
	spi->disableRxIRQ();
	dma_in->enable_tc_irq();

	this->dma_in->setNumberOfData(nbr_of_bytes);
	this->dma_in->enableDMA();
	spi->enableSPI_DMA();
}

void RPI::start_dma_out_communication(uint32_t nbr_of_bytes) {
	dma_out->enable_tc_irq();

	this->dma_out->setNumberOfData(nbr_of_bytes);
	this->dma_out->enableDMA();
	spi->enableSPI_DMA();
}

volatile uint8_t* RPI::get_read_buffer() {
	return read_buffer;
}

RPIDataPackage* RPI::get_read_package() {
	return new RPIDataPackage((uint8_t*)read_buffer);
}

RPIDataPackage* RPI::get_write_package() {
	// +4 to make room for the number of bytes
	return new RPIDataPackage((uint8_t*)write_buffer+4);
}

void RPI::send_package(RPIDataPackage* write_package) {
	//+4 to make room for the number of bytes
	((volatile uint32_t*)write_buffer)[0] = (uint32_t)(write_package->nbr_of_bytes_to_send());
	start_dma_out_communication(write_package->nbr_of_bytes_to_send()+4);
}

