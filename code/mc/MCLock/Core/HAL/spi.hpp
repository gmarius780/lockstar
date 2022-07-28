/*
 * spi.h
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
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
    SPI(uint8_t spi_number, DMA* TxHandler, DMA* RxHandler);
    
    /* Data operations */
    //M: Nicht immer 16 bit?
    int16_t read16bitData() { return (int16_t)SPI_regs->DR; }
    int8_t read8bitData() { return (int8_t)SPI_regs->DR; };
    void writeData(int16_t d);
    volatile uint32_t* getDRAddress() { return &SPI_regs->DR; };
    bool isBusy() { return (bool)(SPI_regs->SR & SPI_SR_BSY); };
    
    /* Configurations */
    void bindDMAHandlers(DMA* TxHandler, DMA* RxHandler);
    void unbindDMAHandlers();

    /* Enables/Disables */
    void enableSPI();
    void disableSPI();
    void enableTxIRQ(); // Tx Buffer empty interrupt
    void disableTxIRQ();
    void enableRxIRQ(); // Rx Buffer not empty interrupt
    void disableRxIRQ();
    void enableSPI_DMA();
    void disableSPI_DMA();
    

private:
    SPI_TypeDef* SPI_regs;
    DMA  *DMATxHandler, *DMARxHandler;
    bool DMAHandlersValid;
    void checkDMAHandlers();

};

#endif /* HAL_SPI_HPP_ */
