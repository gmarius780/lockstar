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

#define SPI_MASTER SPI4 // SPI2, SPI4
#define SPI_SLAVE SPI2                                   // SPI2, SPI4
#define LOOPBACK_MODE 1                                   // 0: no loopback, 1: loopback
#define BAUDRATE_PRESCALER LL_SPI_BAUDRATEPRESCALER_DIV64 // DIV2, DIV4, DIV8, DIV16, DIV32, DIV64, DIV128, DIV256
#define SPI_MASTER_DIRECTION LL_SPI_SIMPLEX_TX            // FULL_DUPLEX, HALF_DUPLEX_TX, HALF_DUPLEX_RX, SIMPLEX_TX, SIMPLEX_RX
#define SPI_SLAVE_DIRECTION LL_SPI_SIMPLEX_RX             // FULL_DUPLEX, HALF_DUPLEX_TX, HALF_DUPLEX_RX, SIMPLEX_TX, SIMPLEX_RX
#define TIMEOUT 0xFFF
#define NSS_MODE_MASTER LL_SPI_NSS_HARD_OUTPUT // LL_SPI_NSS_SOFT, LL_SPI_NSS_HARD_INPUT, LL_SPI_NSS_HARD_OUTPUT
#define NSS_MODE_SLAVE LL_SPI_NSS_HARD_INPUT   // LL_SPI_NSS_SOFT, LL_SPI_NSS_HARD_INPUT, LL_SPI_NSS_HARD_OUTPUT
#define BUFFER_SIZE 24

class SPITestModule
{
public:
    SPITestModule()
    {
    }

    void run()
    {
        SPI_config(SPI_MASTER, SPI_MASTER_DIRECTION, NSS_MODE_MASTER, 0);
        #ifdef SPI_SLAVE
            SPI_config(SPI_SLAVE, SPI_SLAVE_DIRECTION, NSS_MODE_SLAVE, 1);
        #endif

        timeout = TIMEOUT;

        // Start Master transfer
        LL_SPI_StartMasterTransfer(SPI_MASTER);
        // Check result when in loopback mode
        if (LOOPBACK_MODE)
        {
            // Wait for timeout
            while ((timeout != 0))
            {
                if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
                {
                    timeout--;
                }
            }

            if (BufferCmp(SPI_MASTER_TxBuffer, SPI_MASTER_RxBuffer, SPIx_NbDataToTransmit))
            {
                /* Turn the LED 3 on if result wrong */
                turn_LED3_on();
            }
            else
            {
                /* Turn the LED 2 on if result is correct */
                turn_LED2_on();
            }
        }
        turn_LED1_on();
        while (true)
        {
        }
    }
    static uint16_t BufferCmp(uint8_t *pBuffer1, uint8_t *pBuffer2, uint16_t BufferLength)
    {
        while (BufferLength--)
        {
            if ((*pBuffer1) != *pBuffer2)
            {
                return BufferLength;
            }
            pBuffer1++;
            pBuffer2++;
        }

        return 0;
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
    }

    void SPI2_TX_Callback(void)
    {
        /* Write character in Data register.
         * TXP flag is cleared by filling data into TXDR register */
        if (SPI_MASTER == SPI2)
            if(SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_MASTER_DIRECTION == LL_SPI_SIMPLEX_TX)
                LL_SPI_TransmitData8(SPI_MASTER, SPI_MASTER_TxBuffer[SPI_MASTER_TXIDX++]);
        
        #ifdef SPI_SLAVE
            if (SPI_SLAVE == SPI2)
                if(SPI_SLAVE_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_SLAVE_DIRECTION == LL_SPI_SIMPLEX_TX)
                    LL_SPI_TransmitData8(SPI_SLAVE, SPI_SLAVE_TxBuffer[SPI_SLAVE_TXIDX++]);
        #endif
    }

    void SPI2_RX_Callback(void)
    {
        /* Read character in Data register.
         * RXP flag is cleared by reading data in RXDR register */
        if (SPI_MASTER == SPI2)
            if(SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_MASTER_DIRECTION == LL_SPI_SIMPLEX_RX)
                SPI_MASTER_RxBuffer[SPI_MASTER_RXIDX++] = LL_SPI_ReceiveData8(SPI_MASTER);

        #ifdef SPI_SLAVE
            if(SPI_SLAVE == SPI2)
                if(SPI_SLAVE_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_SLAVE_DIRECTION == LL_SPI_SIMPLEX_RX)
                    SPI_SLAVE_RxBuffer[SPI_SLAVE_RXIDX++] = LL_SPI_ReceiveData8(SPI_SLAVE);
        #endif
    }

    void SPI4_TX_Callback(void)
    {
        /* Write character in Data register.
         * TXP flag is cleared by filling data into TXDR register */
        if (SPI_MASTER == SPI4)
            if(SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_MASTER_DIRECTION == LL_SPI_SIMPLEX_TX)
                LL_SPI_TransmitData8(SPI_MASTER, SPI_MASTER_TxBuffer[SPI_MASTER_TXIDX++]);
        
        #ifdef SPI_SLAVE
            if (SPI_SLAVE == SPI4)
                if(SPI_SLAVE_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_SLAVE_DIRECTION == LL_SPI_SIMPLEX_TX)
                    LL_SPI_TransmitData8(SPI_SLAVE, SPI_SLAVE_TxBuffer[SPI_SLAVE_TXIDX++]);
        #endif
    }

    void SPI4_RX_Callback(void)
    {
        /* Read character in Data register.
         * RXP flag is cleared by reading data in RXDR register */
        if (SPI_MASTER == SPI4)
            if(SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_MASTER_DIRECTION == LL_SPI_SIMPLEX_RX)
                SPI_MASTER_RxBuffer[SPI_MASTER_RXIDX++] = LL_SPI_ReceiveData8(SPI_MASTER);

        #ifdef SPI_SLAVE
            if(SPI_SLAVE == SPI4)
                if(SPI_SLAVE_DIRECTION == LL_SPI_FULL_DUPLEX || SPI_SLAVE_DIRECTION == LL_SPI_SIMPLEX_RX)
                    SPI_SLAVE_RxBuffer[SPI_SLAVE_RXIDX++] = LL_SPI_ReceiveData8(SPI_SLAVE);
        #endif
    }

    void SPI_TransferError_Callback(void)
    {
        /* Disable ALL Interrupts */
        LL_SPI_DisableIT_TXP(SPI_MASTER);
        if(SPI_MASTER_DIRECTION == LL_SPI_FULL_DUPLEX)
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

    void SPI_config(SPI_TypeDef *SPIx, uint32_t SPIx_DIRECTION, uint32_t NSS_MODE, bool isSlave){
        LL_SPI_EnableGPIOControl(SPIx);
        LL_SPI_SetTransferSize(SPIx, SPIx_NbDataToTransmit);

        if(isSlave){
            LL_SPI_SetMode(SPIx, LL_SPI_MODE_SLAVE);
            LL_SPI_DisableNSSPulseMgt(SPIx);
        }
        else
            LL_SPI_SetMode(SPIx, LL_SPI_MODE_MASTER);

        LL_SPI_SetTransferDirection(SPIx, SPIx_DIRECTION);
        LL_SPI_SetBaudRatePrescaler(SPIx, BAUDRATE_PRESCALER);

        LL_SPI_SetNSSMode(SPIx, NSS_MODE);
        // enable SPI
        LL_SPI_Enable(SPIx);
        // Wait for SPI activation flag
        while (!LL_SPI_IsEnabled(SPIx))
        {
        }

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
    }

public:
    uint32_t SPI_MASTER_TXIDX = 0;
    uint32_t SPI_MASTER_RXIDX = 0;
    uint32_t SPI_SLAVE_TXIDX = 0;
    uint32_t SPI_SLAVE_RXIDX = 0;

    uint32_t timeout = 0;
    uint8_t SPI_MASTER_TxBuffer[BUFFER_SIZE] = "** SPI_M_OneBoard_IT **";
    uint8_t SPI_MASTER_RxBuffer[BUFFER_SIZE] = {0};
    uint8_t SPI_SLAVE_TxBuffer[BUFFER_SIZE] = "** SPI_S_OneBoard_IT **";
    uint8_t SPI_SLAVE_RxBuffer[BUFFER_SIZE] = {0};
    uint32_t SPIx_NbDataToTransmit = BUFFER_SIZE;
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
        module->SPI4_TX_Callback();
        return;
    }
    if (LL_SPI_IsActiveFlag_RXP(SPI4) && LL_SPI_IsEnabledIT_RXP(SPI4))
    {
        /* Call function Reception Callback */
        module->SPI4_RX_Callback();
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
        module->SPI2_TX_Callback();
        return;
    }
    if (LL_SPI_IsActiveFlag_RXP(SPI2) && LL_SPI_IsEnabledIT_RXP(SPI2))
    {
        /* Call function Reception Callback */
        module->SPI2_RX_Callback();
        return;
    }
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
