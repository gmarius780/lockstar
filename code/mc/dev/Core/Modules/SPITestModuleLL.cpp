/*
 * SPITestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../HAL/leds.hpp"

#ifdef SPI_TEST_MODULE
#pragma message("Compiling SPI Test Module LL")
#include "config.h"
#include "stdio.h"

class SPITestModule
{
public:
    SPITestModule()
    {
    }

    void run()
    {

        init_buffer((uint8_t *)SPI_MASTER_TxBuffer, SEED);
        init_buffer((uint8_t *)SPI_SLAVE_TxBuffer, SEED + 1);

#ifdef SPI_SLAVE
        SPI_config(SPI_SLAVE, SPI_SLAVE_DIRECTION, NSS_MODE_SLAVE, 1);
#endif

        SPI_config(SPI_MASTER, SPI_MASTER_DIRECTION, NSS_MODE_MASTER, 0);

        // Start Master transfer
        LL_SPI_StartMasterTransfer(SPI_MASTER);
        while (!LL_DMA_IsActiveFlag_TC3(SPI_MASTER_TXDMA) || !LL_DMA_IsActiveFlag_TC2(SPI_MASTER_RXDMA))
        {
        }
        turn_LED1_on();
        check_result();
        while (true)
        {
        }
    }

    static uint16_t BufferCmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint32_t BufferLength)
    {
        for (uint16_t i = BufferLength; i > 0; i--)
        {
            if ((*pBuffer1) != *pBuffer2)
            {
                return i;
            }
            pBuffer1++;
            pBuffer2++;
        }

        return 0;
    }

    void check_result()
    {
        // Check result when in loopback mode
#ifdef LOOPBACK_MODE
        if (BufferCmp((uint8_t *)SPI_MASTER_TxBuffer, (uint8_t *)SPI_MASTER_RxBuffer, BUFFER_SIZE))
        {
            /* Turn the LED 3 on if result wrong */
            turn_LED3_on();
            printf("SPI Master Tx and Rx buffer are different\n");
        }
        else
        {
            /* Turn the LED 2 on if result is correct */
            turn_LED2_on();
            printf("SPI Master Tx and Rx buffer are the same\n");
        }
#endif
#ifdef MASTERTX_SLAVERX_TEST
        if (BufferCmp(SPI_MASTER_TxBuffer, SPI_SLAVE_RxBuffer, BUFFER_SIZE))
        {
            /* Turn the LED 3 on if result wrong */
            turn_LED3_on();
        }
        else
        {
            /* Turn the LED 2 on if result is correct */
            turn_LED2_on();
        }
#endif
        return;
    }

    void SPI_EOT_Callback(SPI_TypeDef *SPIx)
    {
        LL_SPI_Disable(SPIx);
        LL_SPI_DisableIT_TXP(SPIx);
        LL_SPI_DisableIT_RXP(SPIx);
        LL_SPI_DisableIT_CRCERR(SPIx);
        LL_SPI_DisableIT_OVR(SPIx);
        LL_SPI_DisableIT_UDR(SPIx);
        LL_SPI_DisableIT_EOT(SPIx);
        check_result();
    }

    void SPI_TX_Callback(SPI_TypeDef *SPIx)
    {
        /* Write character in Data register.
         * TXP flag is cleared by filling data into TXDR register */
        if (SPI_MASTER == SPIx)
            if (SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_MASTER_DIRECTION == LL_SPI_SIMPLEX_TX)
                LL_SPI_TransmitData8(SPI_MASTER, SPI_MASTER_TxBuffer[SPI_MASTER_TXIDX++]);

#ifdef SPI_SLAVE
        if (SPI_SLAVE == SPIx)
            if (SPI_SLAVE_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_SLAVE_DIRECTION == LL_SPI_SIMPLEX_TX)
                LL_SPI_TransmitData8(SPI_SLAVE, SPI_SLAVE_TxBuffer[SPI_SLAVE_TXIDX++]);
#endif
    }

    void SPI_RX_Callback(SPI_TypeDef *SPIx)
    {
        /* Read character in Data register.
         * RXP flag is cleared by reading data in RXDR register */
        if (SPI_MASTER == SPIx)
            if (SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_MASTER_DIRECTION == LL_SPI_SIMPLEX_RX)
                SPI_MASTER_RxBuffer[SPI_MASTER_RXIDX++] = LL_SPI_ReceiveData8(SPI_MASTER);

#ifdef SPI_SLAVE
        if (SPI_SLAVE == SPIx)
            if (SPI_SLAVE_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_SLAVE_DIRECTION == LL_SPI_SIMPLEX_RX)
                SPI_SLAVE_RxBuffer[SPI_SLAVE_RXIDX++] = LL_SPI_ReceiveData8(SPI_SLAVE);
#endif
    }

    void SPI_TransferError_Callback(void)
    {
        /* Disable ALL Interrupts */
        LL_SPI_DisableIT_TXP(SPI_MASTER);
        if (SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX)
        {
            LL_SPI_DisableIT_RXP(SPI_MASTER);
        }
#ifdef SPI_SLAVE
        LL_SPI_DisableIT_RXP(SPI_SLAVE);
#endif
        LL_SPI_DisableIT_CRCERR(SPI_MASTER);
        LL_SPI_DisableIT_OVR(SPI_MASTER);
        LL_SPI_DisableIT_UDR(SPI_MASTER);
        LL_SPI_DisableIT_EOT(SPI_MASTER);

        /* Disable SPI */
        LL_SPI_Disable(SPI_MASTER);
    }

    void SPI_DMA_EOT_Callback(SPI_TypeDef *SPIx)
    {
        //1. Disable TX Stream
        while(!LL_SPI_IsActiveFlag_TXC(SPIx)){
        }
        LL_DMA_DisableStream(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM);
        while(LL_SPI_IsActiveFlag_RXWNE(SPIx) || LL_SPI_GetRxFIFOPackingLevel(SPIx)){
        }
        //2. Poll if RX FIFO empty
        LL_DMA_DisableStream(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM);
        while(LL_DMA_IsEnabledStream(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM) || LL_DMA_IsEnabledStream(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM)){
        }
        LL_DMA_DisableIT_TC(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM);
        LL_SPI_Disable(SPIx);
        while (LL_SPI_IsEnabled(SPIx))
        {
        }  
        LL_SPI_DisableDMAReq_TX(SPIx);
        LL_SPI_DisableDMAReq_RX(SPIx);
    }

    void SPI_config(SPI_TypeDef *SPIx, uint32_t SPIx_DIRECTION, uint32_t NSS_MODE, bool isSlave)
    {
        LL_SPI_EnableGPIOControl(SPIx);

        LL_SPI_SetTransferDirection(SPIx, SPIx_DIRECTION);

        LL_SPI_SetBaudRatePrescaler(SPIx, BAUDRATE_PRESCALER);
        LL_SPI_SetNSSMode(SPIx, NSS_MODE);

        if (isSlave)
        {
            LL_SPI_SetMode(SPIx, LL_SPI_MODE_SLAVE);
            LL_SPI_DisableNSSPulseMgt(SPIx);
        }
        else
        {
            LL_SPI_SetMode(SPIx, LL_SPI_MODE_MASTER);
            LL_SPI_EnableNSSPulseMgt(SPIx);
        }
#ifdef PACKET_MODE
        LL_SPI_SetTransferSize(SPIx, BUFFER_SIZE);
#endif
#ifdef DMA_MODE
        LL_DMA_ConfigAddresses(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM, (uint32_t)SPI_MASTER_TxBuffer, LL_SPI_DMA_GetTxRegAddr(SPI_MASTER), LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetDataLength(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM, BUFFER_SIZE);
        LL_DMA_ConfigAddresses(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM, LL_SPI_DMA_GetRxRegAddr(SPI_MASTER), (uint32_t)SPI_MASTER_RxBuffer, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
        LL_DMA_SetDataLength(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM, BUFFER_SIZE);

        LL_DMA_SetStreamPriorityLevel(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetStreamPriorityLevel(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM, LL_DMA_PRIORITY_LOW);

#ifdef USE_FIFOS
        // FIFO mode
        LL_DMA_EnableFifoMode(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM);
        LL_DMA_EnableFifoMode(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM);
        LL_DMA_SetFIFOThreshold(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM, LL_DMA_FIFOTHRESHOLD_FULL);
        LL_DMA_SetFIFOThreshold(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM, LL_DMA_FIFOTHRESHOLD_FULL);
        LL_SPI_SetFIFOThreshold(SPIx, LL_SPI_FIFO_TH_08DATA);
#endif

        LL_DMA_EnableIT_TC(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM);
        LL_SPI_EnableDMAReq_RX(SPI_MASTER);
        LL_DMA_EnableStream(SPI_MASTER_TXDMA, SPI_MASTER_TXDMA_STREAM);
        LL_DMA_EnableStream(SPI_MASTER_RXDMA, SPI_MASTER_RXDMA_STREAM);
        LL_SPI_EnableDMAReq_TX(SPI_MASTER);
#endif

        // enable SPI
        LL_SPI_Enable(SPIx);
        // Wait for SPI activation flag
        while (!LL_SPI_IsEnabled(SPIx))
        {
        }

#ifdef PACKET_MODE
        switch (SPIx_DIRECTION)
        {
        case LL_SPI_FULL_DUPLEX:
            LL_SPI_EnableIT_TXP(SPIx);
            LL_SPI_EnableIT_RXP(SPIx);
            break;
        case LL_SPI_SIMPLEX_RX:
            LL_SPI_EnableIT_RXP(SPIx);
            break;
        case LL_SPI_SIMPLEX_TX:
            LL_SPI_EnableIT_TXP(SPIx);
            break;

        default:
            LL_SPI_EnableIT_TXP(SPIx);
            LL_SPI_EnableIT_RXP(SPIx);
            break;
        }

        LL_SPI_EnableIT_CRCERR(SPIx);
        LL_SPI_EnableIT_UDR(SPIx);
        LL_SPI_EnableIT_OVR(SPIx);
        LL_SPI_EnableIT_EOT(SPIx);
#endif
    }

    void init_buffer(uint8_t *buffer, uint32_t seed)
    {

        srand(seed);

        for (int i = 0; i < BUFFER_SIZE - 1; ++i)
        {
            // Generate a random character between 'A' (65) and 'Z' (90)
            buffer[i] = (char)(rand() % 26 + 65);
        }
        buffer[BUFFER_SIZE - 1] = '\0';
    }

public:
    uint32_t SPI_MASTER_TXIDX = 0;
    uint32_t SPI_MASTER_RXIDX = 0;
    uint32_t SPI_SLAVE_TXIDX = 0;
    uint32_t SPI_SLAVE_RXIDX = 0;

    uint8_t SPI_MASTER_TxBuffer[BUFFER_SIZE] = {0};
    uint8_t SPI_MASTER_RxBuffer[BUFFER_SIZE] = {0};
    uint8_t SPI_SLAVE_TxBuffer[BUFFER_SIZE] = {0};
    uint8_t SPI_SLAVE_RxBuffer[BUFFER_SIZE] = {0};
};

SPITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/

void SPI4_IRQHandler(void)
{
    /* Check OVR/UDR flag value in ISR register */
    if (LL_SPI_IsActiveFlag_OVR(SPI4) || LL_SPI_IsActiveFlag_UDR(SPI4))
    {
        /* Call Error function */
        module->SPI_TransferError_Callback();
    }
    /* Check EOT flag value in ISR register */
    if (LL_SPI_IsActiveFlag_EOT(SPI4) && LL_SPI_IsEnabledIT_EOT(SPI4))
    {
        /* Call function Reception Callback */
        module->SPI_EOT_Callback(SPI4);
        return;
    }
    /* Check TXP flag value in ISR register */
    if ((LL_SPI_IsActiveFlag_TXP(SPI4) && LL_SPI_IsEnabledIT_TXP(SPI4)))
    {
        /* Call function Reception Callback */
        module->SPI_TX_Callback(SPI4);
        return;
    }
    if (LL_SPI_IsActiveFlag_RXP(SPI4) && LL_SPI_IsEnabledIT_RXP(SPI4))
    {
        /* Call function Reception Callback */
        module->SPI_RX_Callback(SPI4);
        return;
    }
}

void SPI2_IRQHandler(void)
{
    /* Check OVR/UDR flag value in ISR register */
    if (LL_SPI_IsActiveFlag_OVR(SPI2) || LL_SPI_IsActiveFlag_UDR(SPI2))
    {
        /* Call Error function */
        module->SPI_TransferError_Callback();
    }
    /* Check EOT flag value in ISR register */
    if (LL_SPI_IsActiveFlag_EOT(SPI2) && LL_SPI_IsEnabledIT_EOT(SPI2))
    {
        /* Call function Reception Callback */
        module->SPI_EOT_Callback(SPI2);
        return;
    }
    /* Check TXP flag value in ISR register */
    if ((LL_SPI_IsActiveFlag_TXP(SPI2) && LL_SPI_IsEnabledIT_TXP(SPI2)))
    {
        /* Call function Reception Callback */
        module->SPI_TX_Callback(SPI2);
        return;
    }
    if (LL_SPI_IsActiveFlag_RXP(SPI2) && LL_SPI_IsEnabledIT_RXP(SPI2))
    {
        /* Call function Reception Callback */
        module->SPI_RX_Callback(SPI2);
        return;
    }
}

// void DMA1_Stream0_IRQHandler(void){

// }
void DMA2_Stream1_IRQHandler(void){
    module->SPI_DMA_EOT_Callback(SPI2);
    return;
}

void DMA1_Stream3_IRQHandler(void){
    module->SPI_DMA_EOT_Callback(SPI2);
    return;
}

/******************************
 *       MAIN FUNCTION        *
 ******************************/
void start(void)
{
    /* To speed up the access to functions, that are often called, we store them in the RAM instead of the FLASH memory.
     * RAM is volatile. We therefore need to load the code into RAM at startup time. For background and explanations,
     * check https://rhye.org/post/stm32-with-opencm3-4-memory-sections/
     * */
    extern unsigned __sram_func_start, __sram_func_end, __sram_func_loadaddr;
    volatile unsigned *src = &__sram_func_loadaddr;
    volatile unsigned *dest = &__sram_func_start;
    while (dest < &__sram_func_end)
    {
        *dest = *src;
        src++;
        dest++;
    }

    /* After power on, give all devices a moment to properly start up */
    HAL_Delay(200);

    module = new SPITestModule();

    module->run();
}
#endif
