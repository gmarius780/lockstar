/*
 * dma_new.hpp
 *
 *  Created on: Jul 7, 2022
 *      Author: sjele
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
} DMA_config_t;

// All reserved bits in CR register (represented as 1: reserved, 0: rw)
#define DMA_CR_RESERVED_BITS_MASK 0xF0100000

class DMA {
public:
	DMA(DMA_config_t config);

    /* Enables/Disables */
    void enableDMA();
    void disableDMA();

private:
    DMA_Stream_TypeDef* DMA_regs;
    uint32_t TCIFBit;
    volatile uint32_t *clear_ISR;
};

#endif /* HAL_DMA_NEW_HPP_ */
