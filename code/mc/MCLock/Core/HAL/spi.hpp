/*
 * spi.h
 *
 *  Created on: Jul 7, 2022
 *      Author: sjele
 */

#ifndef HAL_SPI_HPP_
#define HAL_SPI_HPP_

#include "stm32f427xx.h"
#include "dma_new.hpp"

class SPI {

public:
    
    /* Initialization*/
    // Constructor for SPI w/o DMA
	SPI(uint8_t spi_number);
    // Constructor for SPI with DMA
    SPI(uint8_t spi_number, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, uint32_t DMAprio);
    
    /* Data operations */
    int16_t readData();
    void writeData(int16_t d);
    void armDMA(uint32_t* destinationAddress, uint32_t* sourceAddress, uint16_t bufferSize);
    void disarmDMA();
    
    /* Enables/Disables */
    void enableSPI();
    void disableSPI();
    void enableTxIRQ();
    void disableTxIRQ();
    void enableRxIRQ();
    void disableRxIRQ();
    void enableSPI_DMA();
    void disableSPI_DMA();
    

private:
    SPI_TypeDef* SPI_regs;
    DMA  *DMATxHandler, *DMARxHandler;
};

#endif /* HAL_SPI_HPP_ */
