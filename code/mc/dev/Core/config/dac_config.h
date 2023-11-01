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

#define DAC1_CLEAR_PORT CLR6_GPIO_Port
#define DAC1_CLEAR_PIN CLR6_Pin


#define USE_SPI6


#define SAMPLES 1100

#ifdef USE_SPI2
  #define DAC2_SPI SPI2
  #define DAC2_DMA DMA1
  #define DAC2_DMA_STREAM LL_DMA_STREAM_3
  #define DMAMUX_REQ_DAC_SPI_TX LL_DMAMUX1_REQ_SPI2_TX
  #define DAC2_TX_DMA_STREAM DMA1_Stream3
  #define DAC2_SYNC_PORT SPI2_NSS_GPIO_Port
  #define DAC2_SYNC_PIN SPI2_NSS_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC3
#endif

#ifdef USE_SPI5
  #define DAC2_SPI SPI5
  #define DAC2_DMA DMA2
  #define DAC2_DMA_STREAM LL_DMA_STREAM_3
  #define DMAMUX_REQ_DAC_SPI_TX LL_DMAMUX1_REQ_SPI5_TX
  #define DAC2_TX_DMA_STREAM DMA2_Stream3
  #define DAC2_SYNC_PORT DAC_2_Sync_GPIO_Port
  #define DAC2_SYNC_PIN DAC_2_Sync_Pin
  #define TX_DMA_ClearFlag LL_DMA_ClearFlag_TC3
#endif

#ifdef USE_SPI6
  #define IS_BDMA
  #define DAC2_SPI SPI6
  #define DAC2_DMA BDMA
  #define DAC2_DMA_STREAM LL_BDMA_CHANNEL_1
  #define DMAMUX_REQ_DAC_SPI_TX LL_DMAMUX2_REQ_SPI6_TX
  #define DAC2_TX_DMA_STREAM BDMA_Channel1
  #define DAC2_SYNC_PORT DAC_1_Sync_GPIO_Port
  #define DAC2_SYNC_PIN DAC_1_Sync_Pin
  #define TX_DMA_ClearFlag LL_BDMA_ClearFlag_TC1
#endif


static DAC_Device_TypeDef DAC1_conf = {
    .dac_id = 1,
    .isBDMA = true,
    .SPIx = SPI6,
    .BDMAx = BDMA,
    .BDMA_Channelx = BDMA_Channel1,
    .BDMA_InitStruct = &DAC1_DMA_CONF,
    .bdma_clr_flag = LL_BDMA_ClearFlag_TC1,
    .sync_port = DAC_1_Sync_GPIO_Port,
    .sync_pin = DAC_1_Sync_Pin,
    .clear_port = CLR6_GPIO_Port,
    .clear_pin = CLR6_Pin
};

static DAC_Device_TypeDef DAC2_conf = {
    .dac_id = 2,
    .isBDMA = false,
    .SPIx = SPI5,
    .DMAx = DMA2,
    .DMA_Streamx = DMA2_Stream3,
    .DMA_InitStruct = &DAC2_DMA_CONF,
    .dma_clr_flag = LL_DMA_ClearFlag_TC3,
    .sync_port = DAC_2_Sync_GPIO_Port,
    .sync_pin = DAC_2_Sync_Pin,
    .clear_port = CLR5_GPIO_Port,
    .clear_pin = CLR5_Pin

};


// static DAC_Device_TypeDef DAC3_conf = {
//     .dac_id = 3,
//     .isBDMA = false,
//     .SPIx = SPI2,
//     .DMAx = DMA1,
//     .DMA_Streamx = DMA1_Stream3,
//     .DMA_InitStruct = &DAC2_DMA_CONF,
//     .dma_clr_flag = LL_DMA_ClearFlag_TC3,
//     .sync_port = SPI2_NSS_GPIO_Port,
//     .sync_pin = SPI2_NSS_Pin,
//     .clear_port = CLR5_GPIO_Port,
//     .clear_pin = CLR5_Pin

// };



/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* DAC_CONFIG_H */