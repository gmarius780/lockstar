/*
 * spi.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: sjele
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

    DMATxHandler = NULL;
    DMARxHandler = NULL;
}

SPI::SPI(uint8_t spi_number, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, uint32_t DMAprio) {
    switch(spi_number) {
        case 1: this->SPI_regs = SPI1; break;
        case 4: this->SPI_regs = SPI4; break;
        case 5: this->SPI_regs = SPI5; break;
        case 6: this->SPI_regs = SPI6; break;
    }

    DMA_config_t DMA_In, DMA_Out;

    // select the correct DMA channel (0 to 8)
    DMA_In.CR &= ~(DMA_SxCR_CHSEL); // reset 3 bits that define channel
    DMA_In.CR |= DMA_Channel_In * DMA_SxCR_CHSEL_0; // set channel via 3 control bits
    // set the peripheral address to the SPI data register
    DMA_In.PAR = (uint32_t)&(SPI_regs->DR);
    // set stream priority from very low (00) to very high (11)
    DMA_In.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    DMA_In.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMA_In.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMA_In.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "peripheral to memory"
    DMA_In.CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
    // Clear DBM bit
    DMA_In.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMA_In.CR  |= DMA_SxCR_TCIE;

    DMA_In.NDTR = 0;
    DMA_In.M0AR = 0;
    DMA_In.M1AR = 0;

    DMA_In.stream = DMA_Stream_In;
    DMA_In.channel = DMA_Channel_In;

    DMARxHandler = new DMA(DMA_In);

    // select the correct DMA channel (0 to 8)
    DMA_Out.CR &= ~(DMA_SxCR_CHSEL); // reset 3 bits that define channel
    DMA_Out.CR |= DMA_Channel_Out * DMA_SxCR_CHSEL_0; // set channel via 3 control bits
    // set the peripheral address to the SPI data register
    DMA_Out.PAR = (uint32_t)&(SPI_regs->DR);
    // set stream priority from very low (00) to very high (11)
    DMA_Out.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    DMA_Out.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMA_Out.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMA_Out.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "memory to peripheral"
    DMA_Out.CR &= ~DMA_SxCR_DIR_1;
    DMA_Out.CR |= DMA_SxCR_DIR_0;
    // Clear DBM bit
    DMA_Out.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMA_Out.CR  |= DMA_SxCR_TCIE;

    DMA_Out.NDTR = 0;
    DMA_Out.M0AR = 0;
    DMA_Out.M1AR = 0;

    DMA_Out.stream = DMA_Stream_Out;
    DMA_Out.channel = DMA_Channel_Out;

    DMATxHandler = new DMA(DMA_Out);
}

void SPI::enableSPI() { SPI_regs->CR1 |= SPI_CR1_SPE; }

void SPI::disableSPI() { SPI_regs->CR1 &= ~SPI_CR1_SPE; }

void SPI::enableTxIRQ() { SPI_regs->CR2 |= SPI_CR2_TXEIE; }

void SPI::disableTxIRQ() { SPI_regs->CR2 &= ~SPI_CR2_TXEIE; }

void SPI::enableRxIRQ() { SPI_regs->CR2 |= SPI_CR2_RXNEIE; }

void SPI::disableRxIRQ() { SPI_regs->CR2 &= ~SPI_CR2_RXNEIE; }

void SPI::enableSPI_DMA() { SPI_regs->CR2 |= (SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN); }

void SPI::disableSPI_DMA() { SPI_regs->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN); }

