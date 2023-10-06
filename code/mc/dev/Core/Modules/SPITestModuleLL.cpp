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

#define SPI SPI4                                           // SPI1, SPI2, SPI3, SPI4, SPI5, SPI6
#define LOOPBACK_MODE 1                                    // 0: no loopback, 1: loopback
#define BAUDRATE_PRESCALER LL_SPI_BAUDRATEPRESCALER_DIV64 // DIV2, DIV4, DIV8, DIV16, DIV32, DIV64, DIV128, DIV256
#define DIRECTION LL_SPI_FULL_DUPLEX                        // FULL_DUPLEX, HALF_DUPLEX_TX, HALF_DUPLEX_RX, SIMPLEX_TX, SIMPLEX_RX
#define TIMEOUT 0xFFF
#define NSS_MODE LL_SPI_NSS_SOFT                           // LL_SPI_NSS_SOFT, LL_SPI_NSS_HARD_INPUT, LL_SPI_NSS_HARD_OUTPUT
#define BUFFER_SIZE 24

class SPITestModule
{
public:
    SPITestModule()
    {
    }

    void run()
    {
        timeout = TIMEOUT;

        LL_SPI_EnableGPIOControl(SPI);
        LL_SPI_SetTransferSize(SPI, SPIx_NbDataToTransmit);

        LL_SPI_SetTransferDirection(SPI, DIRECTION);
        LL_SPI_SetBaudRatePrescaler(SPI, BAUDRATE_PRESCALER);

        // enable SPI
        LL_SPI_Enable(SPI);

        // Wait for SPI activation flag
        while (!LL_SPI_IsEnabled(SPI))
        {
        }

        LL_SPI_SetNSSMode(SPI, NSS_MODE);
        if(DIRECTION == LL_SPI_SIMPLEX_RX) {
            LL_SPI_EnableIT_RXP(SPI);
        }
        else if(DIRECTION == LL_SPI_SIMPLEX_TX) {
            LL_SPI_EnableIT_TXP(SPI);
        }
        else if(DIRECTION == LL_SPI_FULL_DUPLEX) {
            LL_SPI_EnableIT_TXP(SPI);
            LL_SPI_EnableIT_RXP(SPI);
        }

        LL_SPI_EnableIT_CRCERR(SPI);
        LL_SPI_EnableIT_UDR(SPI);
        LL_SPI_EnableIT_OVR(SPI);
        LL_SPI_EnableIT_EOT(SPI);
        
        // Start Master transfer
        LL_SPI_StartMasterTransfer(SPI);
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

            if (BufferCmp(SPIx_TxBuffer, SPIx_RxBuffer, SPIx_NbDataToTransmit))
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
    void SPI_EOT_Callback(void)
    {
        LL_SPI_Disable(SPI);
        if (DIRECTION == LL_SPI_FULL_DUPLEX)
        {
            LL_SPI_DisableIT_TXP(SPI);
            LL_SPI_DisableIT_RXP(SPI);
        }
        if (DIRECTION == LL_SPI_SIMPLEX_TX)
        {
            LL_SPI_DisableIT_TXP(SPI);
        }
        if (DIRECTION == LL_SPI_SIMPLEX_RX)
        {
            LL_SPI_DisableIT_RXP(SPI);
        }
        LL_SPI_DisableIT_CRCERR(SPI);
        LL_SPI_DisableIT_OVR(SPI);
        LL_SPI_DisableIT_UDR(SPI);
        LL_SPI_DisableIT_EOT(SPI);
    }
    void SPI_Tx_Callback(void)
    {
        /* Write character in Data register.
         * TXP flag is cleared by filling data into TXDR register */
        LL_SPI_TransmitData8(SPI, SPIx_TxBuffer[SPI_TransmitIndex++]);
        // SPIx_RxBuffer[SPI_ReceiveIndex++] = LL_SPI_ReceiveData8(SPI);
    }

    void SPI_Rx_Callback(void)
    {
        /* Read character in Data register.
         * RXP flag is cleared by reading data in RXDR register */
        SPIx_RxBuffer[SPI_ReceiveIndex++] = LL_SPI_ReceiveData8(SPI);
    }

    void SPI_TransferError_Callback(void)
    {
        /* Disable ALL Interrupts */
        LL_SPI_DisableIT_TXP(SPI);
        LL_SPI_DisableIT_RXP(SPI);
        LL_SPI_DisableIT_CRCERR(SPI);
        LL_SPI_DisableIT_OVR(SPI);
        LL_SPI_DisableIT_UDR(SPI);
        LL_SPI_DisableIT_EOT(SPI);

        /* Disable SPI */
        LL_SPI_Disable(SPI);
    }

public:
    uint32_t SPI_TransmitIndex = 0;
    uint32_t SPI_ReceiveIndex = 0;
    uint32_t timeout = 0;
    uint8_t SPIx_TxBuffer[BUFFER_SIZE] = "**** SPI_OneBoard_IT **";
    uint32_t SPIx_NbDataToTransmit = BUFFER_SIZE;
    uint8_t SPIx_RxBuffer[BUFFER_SIZE] = {0};
};

SPITestModule *module;

/******************************
 *         INTERRUPTS          *
 *******************************/
void SPI4_IRQHandler(void)
{
    /* Check OVR/UDR flag value in ISR register */
    if (LL_SPI_IsActiveFlag_OVR(SPI) || LL_SPI_IsActiveFlag_UDR(SPI))
    {
        /* Call Error function */
        module->SPI_TransferError_Callback();
    }
    /* Check EOT flag value in ISR register */
    if (LL_SPI_IsActiveFlag_EOT(SPI) && LL_SPI_IsEnabledIT_EOT(SPI))
    {
        /* Call function Reception Callback */
        module->SPI_EOT_Callback();
        return;
    }
    /* Check TXP flag value in ISR register */
    if ((LL_SPI_IsActiveFlag_TXP(SPI) && LL_SPI_IsEnabledIT_TXP(SPI)))
    {
        /* Call function Reception Callback */
        module->SPI_Tx_Callback();
        return;
    }
    if (LL_SPI_IsActiveFlag_RXP(SPI) && LL_SPI_IsEnabledIT_RXP(SPI))
    {
        /* Call function Reception Callback */
        module->SPI_Rx_Callback();
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
