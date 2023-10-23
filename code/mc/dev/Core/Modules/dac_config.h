#ifndef DAC_CONFIG_H
#define DAC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define DAC1_SPI SPI5
#define DAC1_DMA DMA2
#define DAC1_DMA_STREAM LL_DMA_STREAM_3


#define DAC1_SENH LL_ADC_CHANNEL_10
#define DAC1_SENL LL_ADC_CHANNEL_11
#define DAC2_SENH LL_ADC_CHANNEL_0
#define DAC2_SENL LL_ADC_CHANNEL_1


#define DAC2_SPI SPI6



/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* DAC_CONFIG_H */