#ifndef DMA_CONFIG_H
#define DMA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stm32h7xx_ll_dma.h"

static LL_BDMA_InitTypeDef DAC1_DMA_CONF = {
    .PeriphOrM2MSrcAddress = 0,
    .MemoryOrM2MDstAddress = 0,
    .Direction = LL_BDMA_DIRECTION_MEMORY_TO_PERIPH,
    .Mode = LL_BDMA_MODE_NORMAL,
    .PeriphOrM2MSrcIncMode = LL_BDMA_PERIPH_NOINCREMENT,
    .MemoryOrM2MDstIncMode = LL_BDMA_MEMORY_INCREMENT,
    .PeriphOrM2MSrcDataSize = LL_BDMA_PDATAALIGN_BYTE,
    .MemoryOrM2MDstDataSize = LL_BDMA_MDATAALIGN_BYTE,
    .NbData = 0,
    .PeriphRequest = LL_DMAMUX2_REQ_SPI6_TX,
    .Priority = LL_BDMA_PRIORITY_HIGH
};

static LL_DMA_InitTypeDef DAC2_DMA_CONF = {
    .PeriphOrM2MSrcAddress = 0,
    .MemoryOrM2MDstAddress = 0,
    .Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
    .Mode = LL_DMA_MODE_NORMAL,
    .PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,
    .MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,
    .PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE,
    .MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE,
    .NbData = 3,
    .PeriphRequest = LL_DMAMUX1_REQ_SPI5_TX,
    .Priority = LL_DMA_PRIORITY_MEDIUM,
    .FIFOMode = LL_DMA_FIFOMODE_DISABLE,
    .FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL,
    .MemBurst = LL_DMA_MBURST_SINGLE,
    .PeriphBurst = LL_DMA_PBURST_SINGLE
};

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* DMA_CONFIG_H */