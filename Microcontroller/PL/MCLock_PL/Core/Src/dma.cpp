#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_spi.h"
#include "dma.hpp"
#include "dac.hpp"

#include "main.h"



#define SPI_DEFAULT_TIMEOUT 100U


typedef struct
{
  __IO uint32_t ISR;   /*!< DMA interrupt status register */
  __IO uint32_t Reserved0;
  __IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
} DMA_Base_Registers;


__attribute__((section("sram_func")))
static void DMACallback(DMA_HandleTypeDef *hdma)
{
	// THIS FUNCTION DOESNT GET CALLED AT THE MOMENT!!!

  SPI_HandleTypeDef* hspi = ( SPI_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);

  CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

  HAL_SPI_TxCpltCallback(hspi);
  HAL_SPI_TxRxCpltCallback(hspi);
}


DMAChannel::DMAChannel(SPI_HandleTypeDef* hspi, uint8_t* TxBuffer, uint8_t* RxBuffer, uint16_t Size, bool Direction/*, SPI_Dev* Parent*/) // @suppress("Class members should be properly initialized")
{
	this->Direction = Direction;
	this->hspi = hspi;
	this->RxBuffer = RxBuffer;
	this->TxBuffer = TxBuffer;
	this->Size = Size;

	// Set DMA Handler
	switch(Direction)
	{
	case DIR_OUT:
		this->hdma = hspi->hdmatx;
		break;
	case DIR_BOTH:
		this->hdma = hspi->hdmarx;
		break;
	}


	DMA_Base_Registers *regs;

	switch(Direction)
	{
	case DIR_OUT:
		// Clear all interrupt flags at correct offset within the register
		regs = (DMA_Base_Registers *)hdma->StreamBaseAddress;
	    regs->IFCR = DMA_FLAG_TCIF0_4 << hdma->StreamIndex;
	    regs->IFCR = 0x3FU << hdma->StreamIndex;

		// set Callback
		hdma->XferCpltCallback = DMACallback;
		// Disable DMA to make it programmable
		__HAL_DMA_DISABLE(hdma);
	    // Clear DBM bit
	    hdma->Instance->CR &= (uint32_t)(~DMA_SxCR_DBM);
	    // Configure DMA Stream data length
	    hdma->Instance->NDTR = Size;
	    // Configure DMA Stream source/destination address
	    // destination address
	    hdma->Instance->PAR = (uint32_t)&hspi->Instance->DR;
	    // source address
	    hdma->Instance->M0AR = (uint32_t)TxBuffer;
	    // Program transmission-complete interrupt
	    hdma->Instance->CR  |= DMA_IT_TC;
	    hdma->Instance->FCR |= DMA_IT_FE;
		// Check if the SPI is already enabled

		if((hspi->Instance->CR1 &SPI_CR1_SPE) != SPI_CR1_SPE)
		{
			// Enable SPI peripheral
			__HAL_SPI_ENABLE(hspi);
		}
		// Enable DMA
		__HAL_DMA_ENABLE(hdma);
		__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_ERR));

		break;

	case DIR_BOTH:

		// Disable DMA to make it programmable
		__HAL_DMA_DISABLE(hdma);
		// Clear all interrupt flags at correct offset within the register
		regs = (DMA_Base_Registers *)hdma->StreamBaseAddress;
	    regs->IFCR = DMA_FLAG_TCIF0_4 << hdma->StreamIndex;
	    regs->IFCR = 0x3FU << hdma->StreamIndex;
        hdma->Instance->FCR &= ~(DMA_IT_FE);
        hdma->Instance->CR  &= ~(DMA_IT_TC);


		// set Callback
		hdma->XferCpltCallback = NULL;
	    // Clear DBM bit
	    hdma->Instance->CR &= (uint32_t)(~DMA_SxCR_DBM);
	    // Configure DMA Stream data length
	    hdma->Instance->NDTR = Size;
	    // Configure DMA Stream source/destination address
	    // destination address
	    hdma->Instance->PAR = (uint32_t)&hspi->Instance->DR;
	    // source address
	    hdma->Instance->M0AR = (uint32_t)RxBuffer;
	    // Clear all interrupt flags at correct offset within the register
	    regs->IFCR = 0x3FU << hdma->StreamIndex;
	    // Program transmission-complete interrupt
	    hdma->Instance->CR  |= DMA_IT_TC;
	    hdma->Instance->FCR |= DMA_IT_FE;
		// Check if the SPI is already enabled
	    __HAL_DMA_ENABLE(hdma);

	    this->hdma = hspi->hdmatx;

		// Disable DMA to make it programmable
		__HAL_DMA_DISABLE(hdma);

	    hspi->hdmatx->XferHalfCpltCallback = NULL;
	    hspi->hdmatx->XferCpltCallback     = NULL;
	    hspi->hdmatx->XferErrorCallback    = NULL;
	    hspi->hdmatx->XferAbortCallback    = NULL;

		// Clear all interrupt flags at correct offset within the register
		regs = (DMA_Base_Registers *)hdma->StreamBaseAddress;
	    regs->IFCR = DMA_FLAG_TCIF0_4 << hdma->StreamIndex;
	    regs->IFCR = 0x3FU << hdma->StreamIndex;
	    hdma->Instance->CR  &= ~(DMA_IT_TC);
	    hdma->Instance->FCR &= ~(DMA_IT_FE);

	    // Clear DBM bit
	    hdma->Instance->CR &= (uint32_t)(~DMA_SxCR_DBM);
	    // Configure DMA Stream data length
	    hdma->Instance->NDTR = Size;
	    // Configure DMA Stream source/destination address
	    // destination address
	    hdma->Instance->PAR = (uint32_t)&hspi->Instance->DR;
	    // source address
	    hdma->Instance->M0AR = (uint32_t)TxBuffer;
	    // Clear all interrupt flags at correct offset within the register
	    regs->IFCR = 0x3FU << hdma->StreamIndex;
	    // Program transmission-complete interrupt
	    hdma->Instance->CR  |= DMA_IT_TC;
	    hdma->Instance->FCR |= DMA_IT_FE;
		// Check if the SPI is already enabled

		if((hspi->Instance->CR1 &SPI_CR1_SPE) != SPI_CR1_SPE)
		{
			// Enable SPI peripheral
			__HAL_SPI_ENABLE(hspi);
		}
		// Enable DMA
		__HAL_DMA_ENABLE(hdma);
		__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_ERR));

		break;

	}

}

void DMAChannel::updateRegisters(uint8_t* TxBuffer, uint8_t* RxBuffer, uint16_t Size)
{
	this->RxBuffer = RxBuffer;
	this->TxBuffer = TxBuffer;
	this->Size = Size;
}


__attribute__((section("sram_func")))
void DMAChannel::Fire()
{
	switch(Direction)
	{
	case DIR_OUT:
	{
		// Enable Tx DMA Request
		__HAL_SPI_ENABLE(hspi);
		__HAL_DMA_ENABLE(hdma);
		__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_ERR));
		SET_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);
		break;
	}
	case DIR_BOTH:
		__HAL_DMA_ENABLE(hdma);
		__HAL_SPI_ENABLE(hspi);
		__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_ERR));
		SET_BIT(hspi->Instance->CR2, SPI_CR2_RXDMAEN);
		SET_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);
		break;
	}
}


