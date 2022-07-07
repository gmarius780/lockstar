/*
 * dma_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: sjele
 */

#include "dma_new.hpp"
#include <stddef.h>

DMA::DMA(DMA_config_t config) {
	
    switch(config.stream) {
    case 0: DMA_regs = DMA2_Stream0; clear_ISR = &(DMA2->LIFCR); TCIFBit = 1UL<<5; break;
    case 1: DMA_regs = DMA2_Stream1; clear_ISR = &(DMA2->LIFCR); TCIFBit = 1UL<<11; break;
    case 2: DMA_regs = DMA2_Stream2; clear_ISR = &(DMA2->LIFCR); TCIFBit = 1UL<<21; break;
    case 3: DMA_regs = DMA2_Stream3; clear_ISR = &(DMA2->LIFCR); TCIFBit = 1UL<<27; break;
    case 4: DMA_regs = DMA2_Stream4; clear_ISR = &(DMA2->HIFCR); TCIFBit = 1UL<<5; break;
    case 5: DMA_regs = DMA2_Stream5; clear_ISR = &(DMA2->HIFCR); TCIFBit = 1UL<<11; break;
    case 6: DMA_regs = DMA2_Stream6; clear_ISR = &(DMA2->HIFCR); TCIFBit = 1UL<<21; break;
    case 7: DMA_regs = DMA2_Stream7; clear_ISR = &(DMA2->HIFCR); TCIFBit = 1UL<<27; break;
    default: DMA_regs = NULL;
    }

    disableDMA();
    // Clear all bits that are not reserved
    DMA_regs->CR &= ~DMA_CR_RESERVED_BITS_MASK;
    // Update the DMA configuration register
    DMA_regs->CR |= config.CR;
    // Peripheral address register
    DMA_regs->PAR = config.PAR;
    // Number of data register
    DMA_regs->NDTR = config.NDTR;
    // Memory 0 address
    DMA_regs->M0AR = config.M0AR;
    // Memory 1 address (only used in double buffer mode)
    DMA_regs->M1AR = config.M1AR;

}

void DMA::enableDMA() { DMA_regs->CR |= DMA_SxCR_EN; }

void DMA::disableDMA() { DMA_regs->CR &= ~DMA_SxCR_EN; }
