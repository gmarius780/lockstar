#ifndef DAC_CONFIG_H
#define DAC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dma_config.h"
#define DAC1_SENH LL_ADC_CHANNEL_10
#define DAC1_SENL LL_ADC_CHANNEL_11
#define DAC2_SENH LL_ADC_CHANNEL_0
#define DAC2_SENL LL_ADC_CHANNEL_1

extern ADC_HandleTypeDef hadc3;

__attribute__((section(".dtcmram"))) static DAC_Device_TypeDef DAC1_conf = {
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
    .clear_pin = CLR6_Pin,
    .STM_ADC = &hadc3,
    .SENH = LL_ADC_CHANNEL_10,
    .SENL = LL_ADC_CHANNEL_11};
__attribute__((section(".dtcmram"))) static DAC_Device_TypeDef DAC2_conf = {
    .dac_id = 2,
    .isBDMA = false,
    .SPIx = SPI4,
    .DMAx = DMA2,
    .DMA_Streamx = DMA2_Stream3,
    .DMA_InitStruct = &DAC2_DMA_CONF,
    .dma_clr_flag = LL_DMA_ClearFlag_TC3,
    .sync_port = DAC_2_Sync_GPIO_Port,
    .sync_pin = DAC_2_Sync_Pin,
    .clear_port = CLR5_GPIO_Port,
    .clear_pin = CLR5_Pin,
    .STM_ADC = &hadc3,
    .SENH = LL_ADC_CHANNEL_0,
    .SENL = LL_ADC_CHANNEL_1

};

#ifdef PROBE_SPI
static DAC_Device_TypeDef DAC3_conf = {.dac_id = 3,
                                       .isBDMA = false,
                                       .SPIx = SPI2,
                                       .DMAx = DMA1,
                                       .DMA_Streamx = DMA1_Stream3,
                                       .DMA_InitStruct = &DAC3_DMA_CONF,
                                       .dma_clr_flag = LL_DMA_ClearFlag_TC3,
                                       .sync_port = SPI2_NSS_GPIO_Port,
                                       .sync_pin = SPI2_NSS_Pin,
                                       .clear_port = CLR5_GPIO_Port,
                                       .clear_pin = CLR5_Pin,
                                       .STM_ADC = &hadc3,
                                       .SENH = LL_ADC_CHANNEL_0,
                                       .SENL = LL_ADC_CHANNEL_1};
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* DAC_CONFIG_H */