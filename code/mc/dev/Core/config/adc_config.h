#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dma_config.h"


static ADC_Device_TypeDef ADC_conf = {
  .SPIx = SPI3,
  .DMARx = DMA1,
  .DMATx = DMA1,
  .DMA_StreamRx = DMA1_Stream4,
  .DMA_StreamTx = DMA1_Stream5,
  .DMA_InitStructRx = &ADC_DMA_RX_CONF,
  .DMA_InitStructTx = &ADC_DMA_TX_CONF,
  .dmaRx_clr_flag = LL_DMA_ClearFlag_TC4,
  .dmaTx_clr_flag = LL_DMA_ClearFlag_TC5,
  .cnv_port = SPI3_NSS_GPIO_Port,
  .cnv_pin = SPI3_NSS_Pin,
  .channel1_config = ADC_BIPOLAR_5V,
  .channel2_config = ADC_BIPOLAR_5V
  };


/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* ADC_CONFIG_H */
