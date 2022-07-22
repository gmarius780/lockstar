/*
 * dma_new.hpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#ifndef HAL_DMA_NEW_HPP_
#define HAL_DMA_NEW_HPP_

#include "stm32f427xx.h"

typedef struct DMA_config_struct {
    __IO uint32_t CR;     /*!< DMA stream x configuration register      */
    __IO uint32_t NDTR;   /*!< DMA stream x number of data register     */
    __IO uint32_t PAR;    /*!< DMA stream x peripheral address register */
    __IO uint32_t M0AR;   /*!< DMA stream x memory 0 address register   */
    __IO uint32_t M1AR;   /*!< DMA stream x memory 1 address register   */
    uint8_t channel;
    uint8_t stream;
    uint8_t priority;
} DMA_config_t;

// All reserved bits in CR register (represented as 1: reserved, 0: rw)
#define DMA_CR_RESERVED_BITS_MASK 0xF0100000
// LIFCR reserved bits
#define DMA_LIFCR_RESERVED_BITS_MASK 0xF082F082

class DMA {
public:
	DMA(DMA_config_t config);

	/* Configurations */
	void setMemoryAddress(volatile uint8_t* addr, char M0M1);
	void setPeripheralAddress(volatile uint32_t* addr);
	void setNumberOfData(uint32_t n);
    uint32_t getControlReg() { return DMA_regs->CR; };
    void resetTransferCompleteInterruptFlag();

    /* Enables/Disables */
	void enableCircMode();
	void disableCircMode();
    void enableDMA();
    void disableDMA();

    void enable_tc_irq();
    void disable_tc_irq();

private:
    DMA_Stream_TypeDef* DMA_regs;
    uint32_t TCIFBit;
    volatile uint32_t *LIFCRreg;
    bool enabled;
};

#endif /* HAL_DMA_NEW_HPP_ */
