#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


#define USE_SPI3
#define NORMAL_MODE
// #define DOUBLE_BUFFER
#define SAMPLES 1100

#ifdef USE_SPI2
  #define ADC_SPI SPI2
  #define ADC_DMA_TX DMA1
  #define ADC_DMA_RX DMA1
  #define ADC_DMA_TX_STREAM LL_DMA_STREAM_3
  #define ADC_DMA_RX_STREAM LL_DMA_STREAM_2
  #define LL_DMAMUX1_REQ_ADC_SPI_TX LL_DMAMUX1_REQ_SPI2_TX
  #define LL_DMAMUX1_REQ_ADC_SPI_RX LL_DMAMUX1_REQ_SPI2_RX
  #define ADC_RX_DMA_STREAM DMA1_Stream2
  #define ADC_TX_DMA_STREAM DMA1_Stream3
  #define NSS_PORT SPI2_NSS_GPIO_Port
  #define NSS_PIN SPI2_NSS_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC3
  #define RX_DMA_ClearFlag LL_DMA_ClearFlag_TC2
#endif

#ifdef USE_SPI3
  #define ADC_SPI SPI3
  #define ADC_DMA_TX DMA1
  #define ADC_DMA_RX DMA1
  #define ADC_DMA_TX_STREAM LL_DMA_STREAM_5
  #define ADC_DMA_RX_STREAM LL_DMA_STREAM_4
  #define LL_DMAMUX1_REQ_ADC_SPI_TX LL_DMAMUX1_REQ_SPI3_TX
  #define LL_DMAMUX1_REQ_ADC_SPI_RX LL_DMAMUX1_REQ_SPI3_RX
  #define ADC_RX_DMA_STREAM DMA1_Stream4
  #define ADC_TX_DMA_STREAM DMA1_Stream5
  #define NSS_PORT SPI3_NSS_GPIO_Port
  #define NSS_PIN SPI3_NSS_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC5
  #define RX_DMA_ClearFlag LL_DMA_ClearFlag_TC4
#endif

#define ADC_DMA_MODE LL_DMA_MODE_NORMAL

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* ADC_CONFIG_H */
