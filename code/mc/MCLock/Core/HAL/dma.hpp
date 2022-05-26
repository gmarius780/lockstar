/*
 * newdma.hpp
 *
 *  Created on: 5 Mar 2020
 *      Author: qo
 */

#ifndef HAL_DMA_HPP_
#define HAL_DMA_HPP_

#include "stm32f427xx.h"
#include <stddef.h>
//#define NULL 0
#define NONE 10

class SPI_DMA_Handler
{
	public:
		SPI_DMA_Handler(uint8_t SPI, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, uint32_t priority);
		void Config(uint8_t SPI, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, uint32_t priority);
		void Transfer(uint8_t *ReadBuffer, uint8_t *WriteBuffer, uint16_t BufferSize);
		void Callback();
		void JustWrite();
		void JustRead();
		void RestoreDefault();
		static void enableCircularMode(DMA_Stream_TypeDef*);
		static void setupDMAStream(DMA_Stream_TypeDef* stream, uint8_t* Buffer, uint16_t BufferSize);
		DMA_Stream_TypeDef* getInputStream() { return DMA_In; };
		DMA_Stream_TypeDef* getOutputStream() { return DMA_Out; };

	private:
		volatile uint32_t *Clear_ISR_In, *Clear_ISR_Out;
		DMA_Stream_TypeDef *DMA_In, *DMA_Out, *DMA_In_Backup, *DMA_Out_Backup;
		uint32_t TCIFBit_In, TCIFBit_Out;
		SPI_TypeDef *SPI;
};


#endif /* HAL_DMA_HPP_ */
