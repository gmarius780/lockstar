#ifndef RPI_CONFIG_H
#define RPI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
#include "dma_config.h"
static RPI_TypeDef RPI_conf = {.SPIx = SPI1,
                               .DMARx = DMA1,
                               .DMATx = DMA1,
                               .DMA_StreamRx = DMA1_Stream0,
                               .DMA_StreamTx = DMA1_Stream1,
                               .DMA_InitStructRx = &RPI_DMA_RX_CONF,
                               .DMA_InitStructTx = &RPI_DMA_TX_CONF,
                               .dmaRx_clr_flag = LL_DMA_ClearFlag_TC0,
                               .dmaTx_clr_flag = LL_DMA_ClearFlag_TC1};

#ifdef __cplusplus
}

#endif
#endif /* RPI_CONFIG_H */