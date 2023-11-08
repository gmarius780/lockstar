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

SPI::SPI(SPI_TypeDef *SPIx)
{
	SPI_regs = SPIx;
	// LL_SPI_EnableGPIOControl(SPIx);
	// LL_SPI_SetMode(SPIx, LL_SPI_MODE_MASTER);
    // LL_SPI_DisableNSSPulseMgt(SPIx);
	// LL_SPI_SetNSSMode(SPIx, LL_SPI_NSS_SOFT);
}

void SPI::writeData(uint8_t data)
{
	SPI_regs->TXDR = data;
}

//__attribute__((section("sram_func")))
void SPI::enableSPI_DMA()
{
	SPI_regs->CFG1 |= (SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
}
void SPI::enableMasterTransmit()
{
	SPI_regs->CR1 |= SPI_CR1_CSTART;
}

//__attribute__((section("sram_func")))
void SPI::disableSPI_DMA()
{
	while (~(SPI_regs->SR & SPI_SR_DXP))
	{
	}
	// TODO: implement busy() method
	SPI_regs->CFG1 &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
}

//__attribute__((section("sram_func")))
void SPI::enable_spi_rx_dma()
{
	SPI_regs->CFG1 |= SPI_CFG1_RXDMAEN;
}

//__attribute__((section("sram_func")))
void SPI::disable_spi_rx_dma()
{
	SPI_regs->CFG1 &= ~SPI_CFG1_RXDMAEN;
}

//__attribute__((section("sram_func")))
void SPI::enable_spi_tx_dma()
{
	SPI_regs->CFG1 |= SPI_CFG1_TXDMAEN;
}

//__attribute__((section("sram_func")))
void SPI::disable_spi_tx_dma()
{
	while (~(SPI_regs->SR & SPI_SR_DXP))
	{
	}
	SPI_regs->CR2 &= ~SPI_CFG1_TXDMAEN;
}

void SPI::enableSPI()
{
	SPI_regs->CR1 |= SPI_CR1_SPE;
	while (!LL_SPI_IsEnabled(SPI_regs))
	{
	}
}

void SPI::disableSPI()
{
	SPI_regs->CR1 &= ~SPI_CR1_SPE;
}

void SPI::enableTxIRQ()
{
	SPI_regs->IER |= SPI_IER_TXPIE;
}

void SPI::disableTxIRQ()
{
	SPI_regs->IER &= ~SPI_IER_TXPIE;
}

void SPI::enableRxIRQ()
{
	SPI_regs->IER |= SPI_IER_RXPIE;
}

void SPI::disableRxIRQ()
{
	SPI_regs->IER &= ~SPI_IER_RXPIE;
}
