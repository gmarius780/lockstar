/*
 * dma_new.hpp
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#ifndef HAL_DMA_HPP_
#define HAL_DMA_HPP_

#include "stm32h725xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_bdma.h"
#include "dma_config.h"

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

// Reserved bits (represented as 1: reserved, 0: not reserved)
#define DMA_CR_RESERVED_BITS_MASK 0xF0100000 	// CR
#define DMA_LIFCR_RESERVED_BITS_MASK 0xF082F082 // LIFCR

class DMA {
public:
	DMA(DMA_HandleTypeDef* hdmaSPI, DMA_config_t config);
    DMA(DMA_TypeDef *DMAx, uint32_t Stream, LL_DMA_InitTypeDef* configuration);
    DMA(BDMA_TypeDef *DMAx, uint32_t Channel, LL_BDMA_InitTypeDef* configuration);

	/* Configurations */
	uint32_t getControlReg();
	void resetTransferCompleteInterruptFlag();
	bool transfer_complete();
	/* NOTE: DMA has to be disabled before using following methods! */
	void setMemory0Address(volatile uint8_t* addr);
	void setMemory1Address(volatile uint8_t* addr);
	void setPeripheralAddress(volatile uint32_t* addr);
	void setNumberOfData(uint32_t n);
	uint32_t getNumberOfData();


    /* Enables/Disables */
	void enableCircMode();
	void disableCircMode();
    void enableDMA();
    void disableDMA();

    void enable_tc_irq();
    void disable_tc_irq();

private:
    DMA_HandleTypeDef* hdmaSPI;
    uint32_t hdma;
    DMA_Stream_TypeDef *DMA_regs;
    BDMA_Channel_TypeDef *BDMA_regs;
    uint32_t TCIFBit;
    volatile uint32_t *IFCRreg;
    volatile uint32_t *interrupt_status_reg;
    uint32_t transfer_complete_bit;
    bool enabled;
    uint32_t tc_flag;
    bool is_dma_instance;
};


__STATIC_INLINE void EnableIT_TC(BDMA_Channel_TypeDef *BDMA_Channel)
{

  SET_BIT(BDMA_Channel->CCR, BDMA_CCR_TCIE);
}

__STATIC_INLINE void EnableIT_TC(DMA_Stream_TypeDef *DMA_Stream)
{
  SET_BIT(DMA_Stream->CR, DMA_SxCR_TCIE);
}

__STATIC_INLINE void EnableChannel(BDMA_Channel_TypeDef *BDMA_Channel)
{
  SET_BIT(BDMA_Channel->CCR, BDMA_CCR_EN);
}

__STATIC_INLINE uint32_t IsEnabledChannel(BDMA_Channel_TypeDef *BDMA_Channel)
{
  return ((READ_BIT(BDMA_Channel->CCR, BDMA_CCR_EN) == (BDMA_CCR_EN)) ? 1UL : 0UL);
}

__STATIC_INLINE void EnableChannel(DMA_Stream_TypeDef *DMA_Stream)
{
  SET_BIT(DMA_Stream->CR, DMA_SxCR_EN);
}
__STATIC_INLINE uint32_t IsEnabledChannel(DMA_Stream_TypeDef *DMA_Stream)
{
  return ((READ_BIT(DMA_Stream->CR, DMA_SxCR_EN) == (DMA_SxCR_EN)) ? 1UL : 0UL);
}

__STATIC_INLINE void DisableChannel(BDMA_Channel_TypeDef *BDMA_Channel)
{
  CLEAR_BIT(BDMA_Channel->CCR, BDMA_CCR_EN);
}
__STATIC_INLINE void DisableChannel(DMA_Stream_TypeDef *DMA_Stream)
{
  CLEAR_BIT(DMA_Stream->CR, DMA_SxCR_EN);
}

__STATIC_INLINE void SetDataLength(BDMA_Channel_TypeDef *BDMA_Channel, uint32_t NbData)
{
  MODIFY_REG(BDMA_Channel->CNDTR, BDMA_CNDTR_NDT, NbData);
}
__STATIC_INLINE void SetDataLength(DMA_Stream_TypeDef *DMA_Stream, uint32_t NbData)
{
  MODIFY_REG(DMA_Stream->NDTR, DMA_SxNDT, NbData);
}

#endif /* HAL_DMA_HPP_ */
