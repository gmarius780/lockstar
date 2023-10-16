#ifndef STM32H7xx_LL_SPI_H
#define STM32H7xx_LL_SPI_H

#ifdef __cplusplus
extern "C" {
#endif



__STATIC_INLINE void writeData(SPI_TypeDef *SPIx, uint8_t TxData)
{
    *((__IO uint8_t *)&SPIx->TXDR) = TxData;
}


__STATIC_INLINE void enableMasterTransmit() {
  SET_BIT(SPIx->CR1, SPI_CR1_CSTART);
}













/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32H7xx_LL_SPI_H */
