/*
 * rpi.cpp
 *
 *  Created on: Jul 15, 2022
 *      Author: marius
 */

#include "rpi.h"
#include "../HAL/leds.hpp"
#include "main.h"

__attribute__((section(".RPI_W_D1"))) __attribute__((__aligned__(0x1000)))
uint8_t write_D1_buffer[4096] = {0};

__attribute__((section(".RPI_R_D1"))) __attribute__((__aligned__(0x1000)))
uint8_t read_D1_buffer[4096] = {0};

RPI::RPI(RPI_TypeDef *RPI_conf) {
  this->RPI_conf = RPI_conf;
  read_buffer = read_D1_buffer;
  write_buffer = write_D1_buffer;

  /*Configure communication_reset_timer
   * It is used to reset is_comminicating, in case the communication failed
   * */
  comm_reset_timer =
      new BasicTimer(4, COMM_RESET_COUNTER_MAX, COMM_RESET_PRESCALER);
  comm_reset_timer->disable();
  comm_reset_timer->disable_interrupt();
  comm_reset_timer->reset_counter();

  spi = new SPI(SPI1);
  spi->disableSPI();

  this->read_package = new RPIDataPackage();
  this->write_package = new RPIDataPackage();

  RPI_conf->DMA_InitStructRx->PeriphOrM2MSrcAddress =
      (uint32_t) & (RPI_conf->SPIx->RXDR);
  RPI_conf->DMA_InitStructRx->MemoryOrM2MDstAddress = (uint32_t)read_buffer;
  RPI_conf->DMA_InitStructRx->NbData = 255 * READ_NBR_BYTES_MULTIPLIER;

  RPI_conf->DMA_InitStructTx->PeriphOrM2MSrcAddress =
      (uint32_t) & (RPI_conf->SPIx->TXDR);
  RPI_conf->DMA_InitStructTx->MemoryOrM2MDstAddress = (uint32_t)write_buffer;

  LL_DMA_Init(RPI_conf->DMARx, __LL_DMA_GET_STREAM(RPI_conf->DMA_StreamRx),
              RPI_conf->DMA_InitStructRx);
  LL_DMA_Init(RPI_conf->DMATx, __LL_DMA_GET_STREAM(RPI_conf->DMA_StreamTx),
              RPI_conf->DMA_InitStructTx);

  is_communicating = false;
  current_nbr_of_bytes = 0;
  // LL_SPI_SetTransferSize(DAC_conf->SPIx, 3);
  spi->enableRxIRQ();
  RPI_conf->SPIx->CR1 |= SPI_CR1_SPE;
}

RPI::~RPI() {
  // TODO Auto-generated destructor stub
}

void RPI::spi_interrupt() {
  if (is_communicating == false) {
    // get new command from rpi
    comm_reset_timer->enable_interrupt();
    comm_reset_timer->enable();

    current_nbr_of_bytes =
        READ_NBR_BYTES_MULTIPLIER *
        ((uint32_t) * (volatile uint8_t *)spi->getRXDRAddress());
    if (current_nbr_of_bytes != 0) {
      CLEAR_BIT(RPI_conf->SPIx->CR1, SPI_CR1_SPE);
      is_communicating = true;
      this->start_dma_in_communication(this->current_nbr_of_bytes);
    }
  }
}

bool RPI::dma_in_interrupt() {
  RPI_conf->dmaRx_clr_flag(RPI_conf->DMARx);
  this->comm_reset_timer->disable();
  this->comm_reset_timer->disable_interrupt();
  this->comm_reset_timer->reset_counter();

  while (LL_SPI_IsActiveFlag_RXWNE(RPI_conf->SPIx) ||
         LL_SPI_GetRxFIFOPackingLevel(RPI_conf->SPIx)) {
  }

  CLEAR_BIT(RPI_conf->SPIx->CR1, SPI_CR1_SPE);
  CLEAR_BIT(RPI_conf->SPIx->CFG1, SPI_CFG1_RXDMAEN);
  while (LL_SPI_IsEnabled(RPI_conf->SPIx) ||
         LL_SPI_IsEnabledDMAReq_RX(RPI_conf->SPIx)) {
  }
  spi->enableRxIRQ();
  // SET_BIT(RPI_conf->SPIx->CR1, SPI_CR1_SPE);
  is_communicating = false;
  return true;
}

void RPI::comm_reset_timer_interrupt() {
  if (this->is_communicating == true) {
    // reset rpi-communication
    this->is_communicating = false;
    this->current_nbr_of_bytes = 0;
    dma_in_interrupt();
  }
  this->comm_reset_timer->disable();
  this->comm_reset_timer->disable_interrupt();
  this->comm_reset_timer->reset_counter();
  // RPI_conf->SPIx->CR1 |= SPI_CR1_SPE;
}

void RPI::dma_in_error_interrupt() {}

void RPI::dma_out_interrupt() {
  RPI_conf->dmaTx_clr_flag(RPI_conf->DMATx);
  while (!LL_SPI_IsActiveFlag_TXC(RPI_conf->SPIx)) {
  }
  this->dma_out_ready_pin_low();
  CLEAR_BIT(RPI_conf->SPIx->CR1, SPI_CR1_SPE);
  CLEAR_BIT(RPI_conf->SPIx->CFG1, SPI_CFG1_TXDMAEN);
  while (LL_SPI_IsEnabled(RPI_conf->SPIx) ||
         LL_SPI_IsEnabledDMAReq_TX(RPI_conf->SPIx)) {
  }
  spi->enableRxIRQ();
  RPI_conf->SPIx->CR1 |= SPI_CR1_SPE;
  while (!LL_SPI_IsEnabled(RPI_conf->SPIx)) {
  }
}

void RPI::dma_out_error_interrupt() {}

void RPI::start_dma_in_communication(uint32_t nbr_of_bytes) {
  spi->disableRxIRQ();
  EnableIT_TC(RPI_conf->DMA_StreamRx);

  SetDataLength(RPI_conf->DMA_StreamRx, nbr_of_bytes);
  EnableChannel(RPI_conf->DMA_StreamRx);
  LL_SPI_EnableDMAReq_RX(RPI_conf->SPIx);

  RPI_conf->SPIx->CR1 |= SPI_CR1_SPE;
}

void RPI::start_dma_out_communication(uint32_t nbr_of_bytes) {
  EnableIT_TC(RPI_conf->DMA_StreamTx);

  SetDataLength(RPI_conf->DMA_StreamTx, nbr_of_bytes);
  EnableChannel(RPI_conf->DMA_StreamTx);
  LL_SPI_EnableDMAReq_TX(RPI_conf->SPIx);
  while (!LL_SPI_IsEnabledDMAReq_TX(RPI_conf->SPIx)) {
  }

  RPI_conf->SPIx->CR1 |= SPI_CR1_SPE;
  while (!LL_SPI_IsEnabled(RPI_conf->SPIx)) {
  }
  this->dma_out_ready_pin_high();
}

volatile uint8_t *RPI::get_read_buffer() { return read_buffer; }

RPIDataPackage *RPI::get_read_package() {
  this->read_package->set_buffer((uint8_t *)read_buffer);
  return this->read_package;
}

RPIDataPackage *RPI::get_write_package() {
  // +4 to make room for the number of bytes
  this->write_package->set_buffer((uint8_t *)write_buffer + 4);
  return this->write_package;
}

void RPI::send_package(RPIDataPackage *write_package) {
  //+4 to make room for the number of bytes
  ((volatile uint32_t *)write_buffer)[0] =
      (uint32_t)(write_package->nbr_of_bytes_to_send());
  start_dma_out_communication(write_package->nbr_of_bytes_to_send() + 4);
}

void RPI::dma_out_ready_pin_high() { Pi_Int_GPIO_Port->BSRR = Pi_Int_Pin; }

void RPI::dma_out_ready_pin_low() {
  Pi_Int_GPIO_Port->BSRR = ((uint32_t)Pi_Int_Pin) << 16;
}
