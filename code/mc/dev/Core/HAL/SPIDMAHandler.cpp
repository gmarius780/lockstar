/*
 * newdma.cpp
 *
 *  Created on: 5 Mar 2020
 *      Author: qo
 */

#include "SPIDMAHandler.hpp"

SPI_DMA_Handler::SPI_DMA_Handler(uint8_t SPI, uint8_t DMA_Stream_In,
                                 uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out,
                                 uint8_t DMA_Channel_Out, uint32_t priority) {
  Config(SPI, DMA_Stream_In, DMA_Channel_In, DMA_Stream_Out, DMA_Channel_Out,
         priority);
}

void SPI_DMA_Handler::Config(uint8_t SPI, uint8_t DMA_Stream_In,
                             uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out,
                             uint8_t DMA_Channel_Out, uint32_t priority) {
  switch (SPI) {
  case 1:
    this->SPI = SPI1;
    break;
  case 4:
    this->SPI = SPI4;
    break;
  case 5:
    this->SPI = SPI5;
    break;
  case 6:
    this->SPI = SPI6;
    break;
  }

  switch (DMA_Stream_In) {
  case 0:
    DMA_In = DMA2_Stream0;
    Clear_ISR_In = &(DMA2->LIFCR);
    TCIFBit_In = 1UL << 5;
    break;
  case 1:
    DMA_In = DMA2_Stream1;
    Clear_ISR_In = &(DMA2->LIFCR);
    TCIFBit_In = 1UL << 11;
    break;
  case 2:
    DMA_In = DMA2_Stream2;
    Clear_ISR_In = &(DMA2->LIFCR);
    TCIFBit_In = 1UL << 21;
    break;
  case 3:
    DMA_In = DMA2_Stream3;
    Clear_ISR_In = &(DMA2->LIFCR);
    TCIFBit_In = 1UL << 27;
    break;
  case 4:
    DMA_In = DMA2_Stream4;
    Clear_ISR_In = &(DMA2->HIFCR);
    TCIFBit_In = 1UL << 5;
    break;
  case 5:
    DMA_In = DMA2_Stream5;
    Clear_ISR_In = &(DMA2->HIFCR);
    TCIFBit_In = 1UL << 11;
    break;
  case 6:
    DMA_In = DMA2_Stream6;
    Clear_ISR_In = &(DMA2->HIFCR);
    TCIFBit_In = 1UL << 21;
    break;
  case 7:
    DMA_In = DMA2_Stream7;
    Clear_ISR_In = &(DMA2->HIFCR);
    TCIFBit_In = 1UL << 27;
    break;
  default:
    DMA_In = NULL;
  }

  switch (DMA_Stream_Out) {
  case 0:
    DMA_Out = DMA2_Stream0;
    Clear_ISR_Out = &(DMA2->LIFCR);
    TCIFBit_Out = 1UL << 5;
    break;
  case 1:
    DMA_Out = DMA2_Stream1;
    Clear_ISR_Out = &(DMA2->LIFCR);
    TCIFBit_Out = 1UL << 11;
    break;
  case 2:
    DMA_Out = DMA2_Stream2;
    Clear_ISR_Out = &(DMA2->LIFCR);
    TCIFBit_Out = 1UL << 21;
    break;
  case 3:
    DMA_Out = DMA2_Stream3;
    Clear_ISR_Out = &(DMA2->LIFCR);
    TCIFBit_Out = 1UL << 27;
    break;
  case 4:
    DMA_Out = DMA2_Stream4;
    Clear_ISR_Out = &(DMA2->HIFCR);
    TCIFBit_Out = 1UL << 5;
    break;
  case 5:
    DMA_Out = DMA2_Stream5;
    Clear_ISR_Out = &(DMA2->HIFCR);
    TCIFBit_Out = 1UL << 11;
    break;
  case 6:
    DMA_Out = DMA2_Stream6;
    Clear_ISR_Out = &(DMA2->HIFCR);
    TCIFBit_Out = 1UL << 21;
    break;
  case 7:
    DMA_Out = DMA2_Stream7;
    Clear_ISR_Out = &(DMA2->HIFCR);
    TCIFBit_Out = 1UL << 27;
    break;
  default:
    DMA_Out = NULL;
  }

  DMA_In_Backup = DMA_In;
  DMA_Out_Backup = DMA_Out;

  // disable the SPI interface
  this->SPI->CR1 &= ~SPI_CR1_SPE;

  // configure input DMA
  if (DMA_In != NULL) {
    // make sure DMA is disabled, otherwise it is not programmable
    DMA_In->CR &= ~DMA_SxCR_EN;

    // set the DMA request ID to the SPI RX request
    DMAMUX1_Channel0->CCR &= ~DMAMUX_CxCR_DMAREQ_ID;
    DMAMUX1_Channel0->CCR |= DMA_Channel_In << DMAMUX_CxCR_DMAREQ_ID_Pos;

    // set the peripheral address to the SPI data register
    DMA_In->PAR = (uint32_t) & (this->SPI->RXDR);
    // set stream priority from very low (00) to very high (11)
    DMA_In->CR &= ~(DMA_SxCR_PL);           // reset 2 bits that define priority
    DMA_In->CR |= priority * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMA_In->CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMA_In->CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "peripheral to memory"
    DMA_In->CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
    // Clear DBM bit
    DMA_In->CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMA_In->CR |= DMA_SxCR_TCIE;
  }

  // configure output DMA
  if (DMA_Out != NULL) {
    // make sure DMA is disabled, otherwise it is not programmable
    DMA_Out->CR &= ~DMA_SxCR_EN;

    // set the DMA request ID to the SPI TX request
    DMAMUX1_Channel1->CCR &= ~DMAMUX_CxCR_DMAREQ_ID;
    DMAMUX1_Channel1->CCR |= DMA_Channel_Out << DMAMUX_CxCR_DMAREQ_ID_Pos;

    // set the peripheral address to the SPI data register
    DMA_Out->PAR = (uint32_t) & (this->SPI->TXDR);
    // set stream priority from very low (00) to very high (11)
    DMA_Out->CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    DMA_Out->CR |= priority * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMA_Out->CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMA_Out->CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "memory to peripheral"
    DMA_Out->CR &= ~DMA_SxCR_DIR_1;
    DMA_Out->CR |= DMA_SxCR_DIR_0;
    // Clear DBM bit
    DMA_Out->CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMA_Out->CR |= DMA_SxCR_TCIE;
  }

  // enable the SPI interface
  this->SPI->CR1 |= SPI_CR1_SPE;
}

__attribute__((section("sram_func"))) void
SPI_DMA_Handler::Transfer(uint8_t *ReadBuffer, uint8_t *WriteBuffer,
                          uint16_t BufferSize) {
  uint32_t ActivateDMAStream = 0;
  if (DMA_Out != NULL) {
    // The memory address is iterated with each transfered byte while the number
    // of items left is decremented. Therefore, both numbers have to be reset
    // with each transfer block point memory address to buffer
    DMA_Out->M0AR = (uint32_t)WriteBuffer;
    // set number of data items
    DMA_Out->NDTR = BufferSize;
    // activate stream
    DMA_Out->CR |= DMA_SxCR_EN;
    // make Tx enabled
    ActivateDMAStream |= SPI_CFG1_TXDMAEN;
  }
  if (DMA_In != NULL) {
    // The memory address is iterated with each transfered byte while the number
    // of items left is decremented. Therefore, both numbers have to be reset
    // with each transfer block point memory address to buffer
    DMA_In->M0AR = (uint32_t)ReadBuffer;
    // set number of data items
    DMA_In->NDTR = BufferSize;
    // activate stream
    DMA_In->CR |= DMA_SxCR_EN;
    // make Rx enabled
    ActivateDMAStream |= SPI_CFG1_RXDMAEN;
  }
  // Tx/Rx DMA is activated
  SPI->CFG1 |= ActivateDMAStream;
  // start SPI transfer
  SPI->CR1 |= SPI_CR1_CSTART;
}

__attribute__((section("sram_func"))) void SPI_DMA_Handler::Callback() {
  // wait while SPI is busy
  while (!(SPI->SR & SPI_SR_DXP))
    ;

  if (DMA_In != NULL) {
    // reset "transmission complete" bit
    *Clear_ISR_In = TCIFBit_In;
    // deactivate DMA - maybe not necessary?
    DMA_In->CR &= ~DMA_SxCR_EN;
    // deactivate SPI Rx DMA
    SPI->CFG1 &= ~SPI_CFG1_RXDMAEN;
  }

  if (DMA_Out != NULL) {
    // reset "transmission complete" bit
    *Clear_ISR_Out = TCIFBit_Out;
    // deactivate DMA - maybe not necessary?
    DMA_Out->CR &= ~DMA_SxCR_EN;
    // deactivate SPI Tx DMA
    SPI->CFG1 &= ~SPI_CFG1_TXDMAEN;
  }
}

void SPI_DMA_Handler::JustWrite() {
  // turn off input stream
  DMA_In = NULL;
}

void SPI_DMA_Handler::JustRead() {
  // turn off input stream
  DMA_Out = NULL;
}

void SPI_DMA_Handler::RestoreDefault() {
  DMA_In = DMA_In_Backup;
  DMA_Out = DMA_Out_Backup;
}
