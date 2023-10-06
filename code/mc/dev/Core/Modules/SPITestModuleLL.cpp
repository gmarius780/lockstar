/*
 * SPITestModule.cpp
 *
 *  Created on: Jul 22, 2022
 *      Author: marius
 */

#include "main.h"
#include "stm32h725xx.h"
#include "stm32h7xx_it.h"
#include "../HAL/spi.hpp"
#include "../HAL/leds.hpp"

#ifdef SPI_TEST_MODULE
#pragma message("Compiling SPI Test Module LL")

class SPITestModule
{
public:
    SPITestModule()
    {
    }

    void run()
    {
        spi_number = 4;
        // TXbuffer[0] = 0x0F;
        // TXbuffer[4] = 0xFF;
        timeout = 0xFFF;
        SPI_Dev = new SPI(spi_number);

        // LL_SPI_SetNSSMode(SPI4, LL_SPI_NSS_HARD_OUTPUT);
        
        LL_SPI_EnableGPIOControl(SPI4);
        LL_SPI_SetTransferSize(SPI4, SPIx_NbDataToTransmit);


        LL_SPI_Enable(SPI4);

        while (!LL_SPI_IsEnabled(SPI4))
        {
        }
        LL_SPI_SetNSSMode(SPI4, LL_SPI_NSS_SOFT);
        LL_SPI_EnableIT_TXP(SPI4);
        LL_SPI_EnableIT_RXP(SPI4);
        LL_SPI_EnableIT_CRCERR(SPI4);
        LL_SPI_EnableIT_UDR(SPI4);
        LL_SPI_EnableIT_OVR(SPI4);
        LL_SPI_EnableIT_EOT(SPI4);

        LL_SPI_StartMasterTransfer(SPI4);
        /* 2 - Wait end of transfer *************************************************/
        while ((timeout != 0))
        {
            if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
            {
                timeout--;
            }
        }

        if (BufferCmp(SPIx_TxBuffer, SPIx_RxBuffer, SPIx_NbDataToTransmit))
        {
            /* Turn the LED 3 on */
            turn_LED3_on();
        }
        else
        {
            /* Turn the LED 2 on */
            turn_LED2_on();
        }
        //__HAL_SPI_ENABLE(hspi4);
        // SPI_Dev->enableSPI();
        // SPI_Dev->enableMasterTransmit();
        while (true)
        {
            //
            // LL_SPI_TransmitData8(SPI4, TXbuffer[0]);
            // LL_SPI_TransmitData8(SPI4, TXbuffer[4]);
            // LL_SPI_StartMasterTransfer(SPI4);
            // while ((timeout != 0))
            // {
            //     if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
            //     {
            //     timeout--;
            //     }
            // }
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
    void SPI4_EOT_Callback(void)
    {
        LL_SPI_Disable(SPI4);
        LL_SPI_DisableIT_TXP(SPI4);
        LL_SPI_DisableIT_CRCERR(SPI4);
        LL_SPI_DisableIT_OVR(SPI4);
        LL_SPI_DisableIT_UDR(SPI4);
        LL_SPI_DisableIT_EOT(SPI4);
    }
    void SPI4_Tx_Callback(void)
    {
        /* Write character in Data register.
         * TXP flag is cleared by filling data into TXDR register */
        LL_SPI_TransmitData8(SPI4, SPIx_TxBuffer[SPI4_TransmitIndex++]);
        //SPIx_RxBuffer[SPI4_ReceiveIndex++] = LL_SPI_ReceiveData8(SPI4);
    }

    void SPI4_Rx_Callback(void)
    {
        /* Read character in Data register.
         * RXP flag is cleared by reading data in RXDR register */
        SPIx_RxBuffer[SPI4_ReceiveIndex++] = LL_SPI_ReceiveData8(SPI4);
    }

    void SPI_TransferError_Callback(void)
    {

        /* Disable ALL Interrupts */
        LL_SPI_DisableIT_TXP(SPI4);
        LL_SPI_DisableIT_RXP(SPI4);
        LL_SPI_DisableIT_CRCERR(SPI4);
        LL_SPI_DisableIT_OVR(SPI4);
        LL_SPI_DisableIT_UDR(SPI4);
        LL_SPI_DisableIT_EOT(SPI4);

        /* Disable SPI4 */
        LL_SPI_Disable(SPI4);
    }

public:
    SPI *SPI_Dev;
    uint8_t spi_number;
    uint32_t SPI4_TransmitIndex = 0;
    uint32_t SPI4_ReceiveIndex = 0;
    uint32_t timeout = 0;
    uint8_t SPIx_TxBuffer[24] = "**** SPI_OneBoard_IT **";
    uint32_t SPIx_NbDataToTransmit = 23;
    uint8_t SPIx_RxBuffer[sizeof(SPIx_TxBuffer)] = {0};
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
        module->SPI4_EOT_Callback();
        return;
    }
    /* Check TXP flag value in ISR register */
    if ((LL_SPI_IsActiveFlag_TXP(SPI4) && LL_SPI_IsEnabledIT_TXP(SPI4)))
    {
        /* Call function Reception Callback */
        module->SPI4_Tx_Callback();
        return;
    }
    if (LL_SPI_IsActiveFlag_RXP(SPI4) && LL_SPI_IsEnabledIT_RXP(SPI4))
    {
        /* Call function Reception Callback */
        module->SPI4_Rx_Callback();
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
