#ifndef DAC_CONFIG_H
#define DAC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DAC1_SENH LL_ADC_CHANNEL_10
#define DAC1_SENL LL_ADC_CHANNEL_11
#define DAC2_SENH LL_ADC_CHANNEL_0
#define DAC2_SENL LL_ADC_CHANNEL_1

#define DAC2_CLEAR_PORT CLR5_GPIO_Port
#define DAC2_CLEAR_PIN CLR5_Pin


#define USE_SPI6


#define SAMPLES 1100

#ifdef USE_SPI2
  #define DAC2_SPI SPI2
  #define DAC2_DMA DMA1
  #define DAC2_DMA_STREAM LL_DMA_STREAM_3
  #define DMAMUX1_REQ_DAC_SPI_TX LL_DMAMUX1_REQ_SPI2_TX
  #define DAC2_TX_DMA_STREAM DMA1_Stream3
  #define DAC2_SYNC_PORT SPI2_NSS_GPIO_Port
  #define DAC2_SYNC_PIN SPI2_NSS_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC3
#endif

#ifdef USE_SPI5
  #define DAC2_SPI SPI5
  #define DAC2_DMA DMA2
  #define DAC2_DMA_STREAM LL_DMA_STREAM_3
  #define DMAMUX1_REQ_DAC_SPI_TX LL_DMAMUX1_REQ_SPI5_TX
  #define DAC2_TX_DMA_STREAM DMA2_Stream3
  #define DAC2_SYNC_PORT DAC_2_Sync_GPIO_Port
  #define DAC2_SYNC_PIN DAC_2_Sync_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC3
#endif

#ifdef USE_SPI6
  #define DAC2_SPI SPI6
  #define DAC2_DMA BDMA
  #define DAC2_DMA_STREAM LL_DMA_STREAM_3
  #define DMAMUX1_REQ_DAC_SPI_TX LL_DMAMUX1_REQ_SPI5_TX
  #define DAC2_TX_DMA_STREAM DMA2_Stream3
  #define DAC2_SYNC_PORT DAC_2_Sync_GPIO_Port
  #define DAC2_SYNC_PIN DAC_2_Sync_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC3
#endif




/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* DAC_CONFIG_H */
