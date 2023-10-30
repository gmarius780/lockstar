/*
 * rpi.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */

#include "rpi.h"
#include "../Inc/main.h"

RPI::RPI() {
	read_buffer = new uint8_t[255*READ_NBR_BYTES_MULTIPLIER];
	write_buffer = new uint8_t[4096];

	/*Configure communication_reset_timer
	 * It is used to reset is_comminicating, in case the communication failed
	 * */
	comm_reset_timer = new BasicTimer(4, COMM_RESET_COUNTER_MAX, COMM_RESET_PRESCALER);
	comm_reset_timer->disable();
	comm_reset_timer->disable_interrupt();
	comm_reset_timer->reset_counter();

	spi = new SPI(SPI1);
	spi->disableSPI();

	LL_DMA_InitTypeDef DMA_RX_InitStruct = {0};
    LL_DMA_InitTypeDef DMA_TX_InitStruct = {0};

    DMA_RX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi->getRXDRAddress();
    DMA_RX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)read_buffer;
    DMA_RX_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_RX_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_RX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_RX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_RX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_RX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_RX_InitStruct.NbData = 255*READ_NBR_BYTES_MULTIPLIER;
    DMA_RX_InitStruct.PeriphRequest = LL_DMAMUX1_REQ_SPI1_RX;
    DMA_RX_InitStruct.Priority = LL_DMA_PRIORITY_HIGH;
    DMA_RX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
    DMA_RX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_RX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_RX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;	

    DMA_TX_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)spi->getTXDRAddress();
    DMA_TX_InitStruct.MemoryOrM2MDstAddress = (uint32_t)write_buffer;
    DMA_TX_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_TX_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_TX_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_TX_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_TX_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_TX_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
    DMA_TX_InitStruct.NbData = 4096;
    DMA_TX_InitStruct.PeriphRequest = LL_DMAMUX1_REQ_SPI1_TX;
    DMA_TX_InitStruct.Priority = LL_DMA_PRIORITY_MEDIUM;
    DMA_TX_InitStruct.FIFOMode = LL_DMA_FIFOMODE_DISABLE;
    DMA_TX_InitStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    DMA_TX_InitStruct.MemBurst = LL_DMA_MBURST_SINGLE;
    DMA_TX_InitStruct.PeriphBurst = LL_DMA_PBURST_SINGLE;

	dma_in = new DMA(DMA1, LL_DMA_STREAM_0, &DMA_RX_InitStruct);
	dma_out = new DMA(DMA1, LL_DMA_STREAM_1, &DMA_TX_InitStruct);

	this->read_package = new RPIDataPackage();
	this->write_package = new RPIDataPackage();

	is_communicating = false;
	current_nbr_of_bytes = 0;
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
		current_nbr_of_bytes = READ_NBR_BYTES_MULTIPLIER * ((uint32_t)*(volatile uint8_t *)spi->getRXDRAddress());
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
	}
	this->comm_reset_timer->disable();
	this->comm_reset_timer->disable_interrupt();
	this->comm_reset_timer->reset_counter();
}

void RPI::dma_in_error_interrupt() {
}

void RPI::dma_out_interrupt() {
	if (dma_out->transfer_complete()) {
		while(spi->isBusy());
		spi->disableSPI_DMA();
		dma_out->disableDMA();
		dma_out->resetTransferCompleteInterruptFlag();
		this->dma_out_ready_pin_low();
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
	this->dma_out_ready_pin_high();
}

volatile uint8_t* RPI::get_read_buffer() {
	return read_buffer;
}

RPIDataPackage* RPI::get_read_package() {
	this->read_package->set_buffer((uint8_t*)read_buffer);
	return this->read_package;
}

RPIDataPackage* RPI::get_write_package() {
	// +4 to make room for the number of bytes
	this->write_package->set_buffer((uint8_t*)write_buffer+4);
	return this->write_package;
}

void RPI::send_package(RPIDataPackage* write_package) {
	//+4 to make room for the number of bytes
	((volatile uint32_t*)write_buffer)[0] = (uint32_t)(write_package->nbr_of_bytes_to_send());
	start_dma_out_communication(write_package->nbr_of_bytes_to_send()+4);
}

void RPI::dma_out_ready_pin_high() {
	Pi_Int_GPIO_Port->BSRR = Pi_Int_Pin;
}

void RPI::dma_out_ready_pin_low() {
	Pi_Int_GPIO_Port->BSRR = ((uint32_t)Pi_Int_Pin)<<16;
}
