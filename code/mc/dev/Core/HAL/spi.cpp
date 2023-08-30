/*
 * spi.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#include "spi.hpp"
#include "stm32h725xx.h"
#include <stddef.h>
#include "dma.hpp"

SPI::SPI(uint8_t spi_number) {
    switch(spi_number) {
        case 1: this->SPI_regs = SPI1; break;
        case 4: this->SPI_regs = SPI4; break;
        case 5: this->SPI_regs = SPI5; break;
        case 6: this->SPI_regs = SPI6; break;
    }
}

void SPI::writeData(int16_t data) { SPI_regs->TXDR = data; }

__attribute__((section("sram_func")))
void SPI::enableSPI_DMA() {
	while(~(SPI_regs->SR & SPI_SR_DXP));
	SPI_regs->CFG1 |= (SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
}

__attribute__((section("sram_func")))
void SPI::disableSPI_DMA() {
	while(~(SPI_regs->SR & SPI_SR_DXP));
	// TODO: implement busy() method
	SPI_regs->CFG1 &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
}

__attribute__((section("sram_func")))
void SPI::enable_spi_rx_dma() {
	SPI_regs->CFG1 |= SPI_CFG1_RXDMAEN;
}

__attribute__((section("sram_func")))
void SPI::disable_spi_rx_dma() {
	SPI_regs->CFG1 &= ~SPI_CFG1_RXDMAEN;
}

__attribute__((section("sram_func")))
void SPI::enable_spi_tx_dma() {
	SPI_regs->CFG1 |= SPI_CFG1_TXDMAEN;
}

__attribute__((section("sram_func")))
void SPI::disable_spi_tx_dma() {
	while(~(SPI_regs->SR & SPI_SR_DXP));
	SPI_regs->CR2 &= ~SPI_CFG1_TXDMAEN;
}

void SPI::enableSPI() { SPI_regs->CR1 |= SPI_CR1_SPE; }

void SPI::disableSPI() { SPI_regs->CR1 &= ~SPI_CR1_SPE; }

void SPI::enableTxIRQ() { SPI_regs->IER |= SPI_IER_TXPIE; }

void SPI::disableTxIRQ() { SPI_regs->IER &= ~SPI_IER_TXPIE; }

void SPI::enableRxIRQ() { SPI_regs->IER |= SPI_IER_RXPIE; }

void SPI::disableRxIRQ() { SPI_regs->IER &= ~SPI_IER_RXPIE; }

