#include "stm32f4xx_hal.h"
#include "spi.h"

#ifndef DMA_H_
#define DMA_H_

#define DIR_OUT	true
#define DIR_BOTH	false

void DMAFinishedCallback(DMA_HandleTypeDef *hdma);

class DMAChannel
{
private:
	DMA_HandleTypeDef *hdma;
	SPI_HandleTypeDef* hspi;
	//SPI_Dev* Parent;
	void (*CallbackFct)(void);
	bool Direction;
	bool Ready;
	uint8_t* RxBuffer;
	uint8_t* TxBuffer;
	uint16_t Size;
public:
	DMAChannel(SPI_HandleTypeDef* hspi, uint8_t* TxBuffer, uint8_t* RxBuffer, uint16_t Size, bool Direction/*, SPI_Dev* Parent*/);
	void updateRegisters(uint8_t* TxBuffer, uint8_t* RxBuffer, uint16_t Size);
	void Fire();
	bool isReady() { return Ready; };
	void Configure();
};

#endif
