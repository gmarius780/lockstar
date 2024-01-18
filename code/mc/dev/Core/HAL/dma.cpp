/*
 * dma_new.cpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#include "dma.hpp"
#include <stddef.h>

DMA::DMA(DMA_HandleTypeDef *hdmaSPI, DMA_config_t config) {

  tc_flag = __HAL_DMA_GET_TC_FLAG_INDEX(hdmaSPI);
  is_dma_instance = IS_DMA_STREAM_INSTANCE(hdmaSPI->Instance);
  hdma = (uint32_t)hdmaSPI;

  this->DMA_regs = (DMA_Stream_TypeDef *)hdmaSPI->Instance;
  this->BDMA_regs = (BDMA_Channel_TypeDef *)hdmaSPI->Instance;

  enabled = false;
  disableDMA();

  // Read the current CR
  uint32_t CR_temp = DMA_regs->CR;
  // Update the DMA configuration register, but leave reserved bits as they are
  DMA_regs->CR = ((CR_temp & DMA_CR_RESERVED_BITS_MASK) |
                  (config.CR & ~DMA_CR_RESERVED_BITS_MASK));
  // Peripheral address register
  DMA_regs->PAR = config.PAR;
  // Number of data register
  DMA_regs->NDTR = config.NDTR;
  // Memory 0 address
  DMA_regs->M0AR = config.M0AR;
  // Memory 1 address (only used in double buffer mode)
  DMA_regs->M1AR = config.M1AR;
}

DMA::DMA(DMA_TypeDef *DMAx, uint32_t Stream,
         LL_DMA_InitTypeDef *configuration) {
  LL_DMA_Init(DMAx, Stream, configuration);
  uint32_t dma_base_addr = (uint32_t)DMAx;
  this->DMA_regs =
      ((DMA_Stream_TypeDef *)(dma_base_addr + LL_DMA_STR_OFFSET_TAB[Stream]));
}

DMA::DMA(BDMA_TypeDef *DMAx, uint32_t Channel,
         LL_BDMA_InitTypeDef *configuration) {
  LL_BDMA_Init(DMAx, Channel, configuration);
  uint32_t dma_base_addr = (uint32_t)DMAx;
  this->BDMA_regs = ((BDMA_Channel_TypeDef *)(dma_base_addr +
                                              LL_BDMA_CH_OFFSET_TAB[Channel]));
}

// __attribute__((section("sram_func")))
void DMA::setMemory0Address(volatile uint8_t *addr) {
  DMA_regs->M0AR = (uint32_t)addr;
}

// __attribute__((section("sram_func")))
void DMA::setMemory1Address(volatile uint8_t *addr) {
  DMA_regs->M1AR = (uint32_t)addr;
}

// __attribute__((section("sram_func")))
void DMA::setPeripheralAddress(volatile uint32_t *addr) {
  DMA_regs->PAR = (uint32_t)addr;
}

// __attribute__((section("sram_func")))
void DMA::setNumberOfData(uint32_t n) { DMA_regs->NDTR = n; }

// __attribute__((section("sram_func")))
uint32_t DMA::getNumberOfData() { return DMA_regs->NDTR; }

void DMA::resetTransferCompleteInterruptFlag() {
  __HAL_DMA_CLEAR_FLAG(hdmaSPI, tc_flag);
}

bool DMA::transfer_complete() { return __HAL_DMA_GET_FLAG(hdmaSPI, tc_flag); }

void DMA::enableCircMode() { DMA_regs->CR |= DMA_SxCR_CIRC; }

void DMA::disableCircMode() { DMA_regs->CR &= ~DMA_SxCR_CIRC; }

// __attribute__((section("sram_func")))
void DMA::enableDMA() {
  DMA_regs->CR |= DMA_SxCR_EN;
  // wait for dma to be disabled
  while (!(DMA_regs->CR & DMA_SxCR_EN))
    ;
  enabled = true;
}

// __attribute__((section("sram_func")))
void DMA::disableDMA() {
  DMA_regs->CR &= ~DMA_SxCR_EN;
  // wait for dma to be enabled otherwise this can lead to errors
  while (DMA_regs->CR & DMA_SxCR_EN)
    ;
  enabled = false;
}

void DMA::disable_tc_irq() { DMA_regs->CR &= ~DMA_SxCR_TCIE; }

void DMA::enable_tc_irq() { DMA_regs->CR |= DMA_SxCR_TCIE; }

uint32_t DMA::getControlReg() { return DMA_regs->CR; }