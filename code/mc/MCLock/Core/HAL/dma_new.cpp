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
    case 0: DMA_regs = DMA2_Stream0; IFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<5; break;
    case 1: DMA_regs = DMA2_Stream1; IFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<11; break;
    case 2: DMA_regs = DMA2_Stream2; IFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<21; break;
    case 3: DMA_regs = DMA2_Stream3; IFCRreg = &(DMA2->LIFCR); TCIFBit = 1UL<<27; break;
    case 4: DMA_regs = DMA2_Stream4; IFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<5; break;
    case 5: DMA_regs = DMA2_Stream5; IFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<11; break;
    case 6: DMA_regs = DMA2_Stream6; IFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<21; break;
    case 7: DMA_regs = DMA2_Stream7; IFCRreg = &(DMA2->HIFCR); TCIFBit = 1UL<<27; break;
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

__attribute__((section("sram_func")))
void DMA::setMemory0Address(volatile uint8_t* addr) {
	DMA_regs->M0AR = (uint32_t) addr;
}

__attribute__((section("sram_func")))
void DMA::setMemory1Address(volatile uint8_t* addr) {
	DMA_regs->M1AR = (uint32_t) addr;
}

__attribute__((section("sram_func")))
void DMA::setPeripheralAddress(volatile uint32_t* addr) {
	DMA_regs->PAR = (uint32_t) addr;
}

__attribute__((section("sram_func")))
void DMA::setNumberOfData(uint32_t n) {
	DMA_regs->NDTR = n;
}

__attribute__((section("sram_func")))
uint32_t DMA::getNumberOfData() {
	return DMA_regs->NDTR;
}

void DMA::resetTransferCompleteInterruptFlag() {
	*(this->IFCRreg) |= this->TCIFBit;
}

void DMA::enableCircMode() {
	DMA_regs->CR |= DMA_SxCR_CIRC;
}

void DMA::disableCircMode() {
	DMA_regs->CR &= ~DMA_SxCR_CIRC;
}

__attribute__((section("sram_func")))
void DMA::enableDMA() {
	DMA_regs->CR |= DMA_SxCR_EN;
	//wait for dma to be disabled
	while(!(DMA_regs->CR & DMA_SxCR_EN));
	enabled = true;
}

__attribute__((section("sram_func")))
void DMA::disableDMA() {
	DMA_regs->CR &= ~DMA_SxCR_EN;
	//wait for dma to be enabled otherwise this can lead to errors
	while(DMA_regs->CR & DMA_SxCR_EN);
	enabled = false;
}

void DMA::disable_tc_irq() {
	DMA_regs->CR  &= ~DMA_SxCR_TCIE;
}

void DMA::enable_tc_irq() {
	DMA_regs->CR  |= DMA_SxCR_TCIE;
}
