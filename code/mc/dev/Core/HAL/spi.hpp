/*
 * spi.h
 *
 *  Created on: Jul 7, 2022
 *      Author: Samuel
 */

#ifndef HAL_SPI_HPP_
#define HAL_SPI_HPP_

#include "stm32h725xx.h"

#include "dma.hpp"
#include "stm32h7xx_ll_spi.h"

class SPI {

public:
    
    /* Initialization*/
    // Constructor for SPI w/o DMA
	SPI(uint8_t spi_number);

    SPI(SPI_TypeDef* SPIx);
    // Constructor for SPI with DMA
    SPI(uint8_t spi_number, DMA* TxHandler, DMA* RxHandler);
    
    /* Data operations */
    //M: Nicht immer 16 bit?
    int16_t read16bitData() { return (int16_t)SPI_regs->RXDR; }
    int8_t read8bitData() { return (int8_t)SPI_regs->RXDR; };
    void writeData(uint8_t d);
    volatile uint32_t* getDRAddress() { return &SPI_regs->RXDR; };
    volatile uint32_t* getRXDRAddress() { return &SPI_regs->RXDR; };
    volatile uint32_t* getTXDRAddress() { return &SPI_regs->TXDR; };
    bool isTxBusy() { return (bool)!(SPI_regs->SR & (SPI_SR_TXP)); }; //there is not enough space to locate next data packet at TxFIFO
    bool isRxBusy() { return (bool)!(SPI_regs->SR & (SPI_SR_RXP)); }; //RxFIFO is empty or a not complete data packet is received
    bool isBusy() { return isRxBusy() || isTxBusy(); };
    

    /* Enables/Disables */
    void enableSPI();
    void disableSPI();
    void enableTxIRQ(); // Tx Buffer empty interrupt
    void disableTxIRQ();
    void enableRxIRQ(); // Rx Buffer not empty interrupt
    void disableRxIRQ();
    void enableSPI_DMA();
    void disableSPI_DMA();
    void enable_spi_rx_dma();
    void disable_spi_rx_dma();
    void enable_spi_tx_dma();
    void disable_spi_tx_dma();
    void enableMasterTransmit();

    

private:
    SPI_TypeDef* SPI_regs;
};

#endif /* HAL_SPI_HPP_ */
