/*
 * dma_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#include "dma_new.hpp"
#include <stddef.h>

DMA::DMA(DMA_config_t config) {
	
    switch(config.stream) {
    case 0: DMA_regs = DMA2_Stream0; LIFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<5; break;
    case 1: DMA_regs = DMA2_Stream1; LIFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<11; break;
    case 2: DMA_regs = DMA2_Stream2; LIFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<21; break;
    case 3: DMA_regs = DMA2_Stream3; LIFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<27; break;
    case 4: DMA_regs = DMA2_Stream4; LIFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<5; break;
    case 5: DMA_regs = DMA2_Stream5; LIFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<11; break;
    case 6: DMA_regs = DMA2_Stream6; LIFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<21; break;
    case 7: DMA_regs = DMA2_Stream7; LIFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<27; break;
    default: DMA_regs = NULL;
    }

    enabled = false;
    disableDMA();

    // Read the current CR
    uint32_t CR_temp = DMA_regs->CR;
    // Update the DMA configuration register, but leave reserved bits as they are
    DMA_regs->CR = ((CR_temp & DMA_CR_RESERVED_BITS_MASK) | (config.CR & ~DMA_CR_RESERVED_BITS_MASK));
    // Peripheral address register
    DMA_regs->PAR = config.PAR;
    // Number of data register
    DMA_regs->NDTR = config.NDTR;
    // Memory 0 address
    DMA_regs->M0AR = config.M0AR;
    // Memory 1 address (only used in double buffer mode)
    DMA_regs->M1AR = config.M1AR;
}

void DMA::setMemoryAddress(volatile uint8_t* addr, char mem) {
	bool wasEnabled = enabled;
	disableDMA();
	if(mem == 0)
		DMA_regs->M0AR = (uint32_t) addr;
	else if(mem == 1)
		DMA_regs->M1AR = (uint32_t) addr;
	if(wasEnabled)
		enableDMA();
}

void DMA::setPeripheralAddress(volatile uint32_t* addr) {
	bool wasEnabled = enabled;
	disableDMA();
	DMA_regs->PAR = (uint32_t) addr;
	if(wasEnabled)
		enableDMA();
}

void DMA::setNumberOfData(uint32_t n) {
	bool wasEnabled = enabled;
	disableDMA();
	DMA_regs->NDTR = n;
	if(wasEnabled)
		enableDMA();
}

void DMA::resetTransferCompleteInterruptFlag() {
    *LIFCRreg |= TCIFBit;
}

void DMA::enableCircMode() { DMA_regs->CR |= DMA_SxCR_CIRC; }

void DMA::disableCircMode() { DMA_regs->CR &= ~DMA_SxCR_CIRC; }

void DMA::enableDMA() { DMA_regs->CR |= DMA_SxCR_EN; enabled = true; }

void DMA::disableDMA() {
	DMA_regs->CR &= ~DMA_SxCR_EN;
	enabled = false;
}
