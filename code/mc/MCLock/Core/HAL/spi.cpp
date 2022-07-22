/*
 * spi.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#include "spi.hpp"
#include "dma_new.hpp"
#include "stm32f427xx.h"
#include <stddef.h>

SPI::SPI(uint8_t spi_number) {
    switch(spi_number) {
        case 1: this->SPI_regs = SPI1; break;
        case 4: this->SPI_regs = SPI4; break;
        case 5: this->SPI_regs = SPI5; break;
        case 6: this->SPI_regs = SPI6; break;
    }

    DMAHandlersValid = false;
    DMATxHandler = NULL;
    DMARxHandler = NULL;
}

SPI::SPI(uint8_t spi_number, DMA* TxHandler, DMA* RxHandler) {
    switch(spi_number) {
        case 1: this->SPI_regs = SPI1; break;
        case 4: this->SPI_regs = SPI4; break;
        case 5: this->SPI_regs = SPI5; break;
        case 6: this->SPI_regs = SPI6; break;
    }

    DMAHandlersValid = false;
    this->DMARxHandler = RxHandler;
    this->DMATxHandler = TxHandler;
    checkDMAHandlers();
}

void SPI::writeData(int16_t data) { SPI_regs->DR = data; }

int16_t SPI::readData() { return SPI_regs->DR; }

void SPI::bindDMAHandlers(DMA* TxHandler, DMA* RxHandler) {
	this->DMARxHandler = DMARxHandler;
	this->DMATxHandler = DMATxHandler;

	//M:richtig?
	//DMATxHandler->setPeripheralAddress(&SPI_regs->DR);
	//DMARxHandler->setPeripheralAddress(&SPI_regs->DR);
	DMATxHandler->setPeripheralAddress(SPI_regs->DR);
	DMARxHandler->setPeripheralAddress(SPI_regs->DR);

	checkDMAHandlers();
}

void SPI::checkDMAHandlers() {
	// TODO: More elaborate checking (e.g. no periph address increment, etc)
	DMAHandlersValid = false;
	if(DMARxHandler != NULL && DMATxHandler != NULL)
		DMAHandlersValid = true;
}

void SPI::unbindDMAHandlers() {
	DMARxHandler = NULL;
	DMATxHandler = NULL;
	DMAHandlersValid = false;
}

void SPI::enableSPI_DMA() {
	if(!DMAHandlersValid)
		return;

	SPI_regs->CR2 |= (SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
}

void SPI::disableSPI_DMA() { SPI_regs->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN); }

void SPI::enableSPI() { SPI_regs->CR1 |= SPI_CR1_SPE; }

void SPI::disableSPI() { SPI_regs->CR1 &= ~SPI_CR1_SPE; }

void SPI::enableTxIRQ() { SPI_regs->CR2 |= SPI_CR2_TXEIE; }

void SPI::disableTxIRQ() { SPI_regs->CR2 &= ~SPI_CR2_TXEIE; }

void SPI::enableRxIRQ() { SPI_regs->CR2 |= SPI_CR2_RXNEIE; }

void SPI::disableRxIRQ() { SPI_regs->CR2 &= ~SPI_CR2_RXNEIE; }

