/*
 * peripheral_configurations.hpp
 *
 *  Created on: 14.07.2022
 *      Author: sjele
 */

#ifndef HAL_PERIPHERAL_CONFIGURATIONS_HPP_
#define HAL_PERIPHERAL_CONFIGURATIONS_HPP_


/*// select the correct DMA channel (0 to 8)
    DMA_In.CR &= ~(DMA_SxCR_CHSEL); // reset 3 bits that define channel
    DMA_In.CR |= DMA_Channel_In * DMA_SxCR_CHSEL_0; // set channel via 3 control bits
    // set the peripheral address to the SPI data register
    DMA_In.PAR = (uint32_t)&(SPI_regs->DR);
    // set stream priority from very low (00) to very high (11)
    DMA_In.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    DMA_In.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMA_In.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMA_In.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "peripheral to memory"
    DMA_In.CR &= ~(DMA_SxCR_DIR_0 | DMA_SxCR_DIR_1);
    // Clear DBM bit
    DMA_In.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMA_In.CR  |= DMA_SxCR_TCIE;

    DMA_In.NDTR = 0;
    DMA_In.M0AR = 0;
    DMA_In.M1AR = 0;

    DMA_In.stream = DMA_Stream_In;
    DMA_In.channel = DMA_Channel_In;

    // select the correct DMA channel (0 to 8)
    DMA_Out.CR &= ~(DMA_SxCR_CHSEL); // reset 3 bits that define channel
    DMA_Out.CR |= DMA_Channel_Out * DMA_SxCR_CHSEL_0; // set channel via 3 control bits
    // set the peripheral address to the SPI data register
    DMA_Out.PAR = (uint32_t)&(SPI_regs->DR);
    // set stream priority from very low (00) to very high (11)
    DMA_Out.CR &= ~(DMA_SxCR_PL); // reset 2 bits that define priority
    DMA_Out.CR |= DMAprio * DMA_SxCR_PL_0; // set priority via 2 control bits
    // increment the memory address with each transfer
    DMA_Out.CR |= DMA_SxCR_MINC;
    // do not increment peripheral address
    DMA_Out.CR &= ~DMA_SxCR_PINC;
    // set direction of transfer to "memory to peripheral"
    DMA_Out.CR &= ~DMA_SxCR_DIR_1;
    DMA_Out.CR |= DMA_SxCR_DIR_0;
    // Clear DBM bit
    DMA_Out.CR &= (uint32_t)(~DMA_SxCR_DBM);
    // Program transmission-complete interrupt
    DMA_Out.CR  |= DMA_SxCR_TCIE;

    DMA_Out.NDTR = 0;
    DMA_Out.M0AR = 0;
    DMA_Out.M1AR = 0;*/


#endif /* HAL_PERIPHERAL_CONFIGURATIONS_HPP_ */
