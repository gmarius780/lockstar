/*
 * raspberrypi.hpp
 *
 *  Created on: 4 Mar 2020
 *      Author: qo
 */

#ifndef INC_RASPBERRYPI_HPP_
#define INC_RASPBERRYPI_HPP_

#include "newdma.hpp"
#include "stm32f4xx_hal.h"

#define DefaultBufferSize	4096

#define RPi_Command_SetIO 16
#define RPi_Command_SetScopeSkips 17
#define RPi_Command_SetPIDParameters 18
#define RPi_Command_MeasureAOMResponse 19
#define RPi_Command_RecordTrace 20
#define RPi_Command_MoveTo 21
#define RPi_Command_GotoLock 22
#define RPi_Command_StopLock 23
#define RPi_Command_DACLog 24
#define RPi_Command_TunePID 25
#define RPi_Command_StartLock 26
#define RPi_Command_DetailedTrace 27
#define RPi_Command_ProgramCalibration 30
#define RPi_Command_OptimizePID 31
#define RPi_Command_SetFFTCorrectionParameters 34
#define RPi_Command_SetChannelEventVoltage 35
#define RPi_Command_SetClockRate 36

#define TunePID_FollowWaypoints 1

class RaspberryPi
{
private:
	SPI_DMA_Handler* DMAHandler;
	GPIO_TypeDef* Int_Port;
	uint16_t Int_Pin;
	volatile bool ready;
public:
	uint8_t *ReadBuffer;
	RaspberryPi(uint8_t SPI, uint8_t DMA_Stream_In, uint8_t DMA_Channel_In, uint8_t DMA_Stream_Out, uint8_t DMA_Channel_Out, GPIO_TypeDef* Int_Port, uint16_t Int_Pin);
	void Transfer(uint8_t *ReadBuffer, uint8_t *WriteBuffer, uint32_t BufferSize);
	void Write(uint8_t *WriteBuffer, uint32_t BufferSize);
	void Read(uint8_t *ReadBuffer, uint32_t BufferSize);
	void Callback();
	void SetIntPin();
	void ResetIntPin();
	bool isReady(){ return ready; };
	// conversion functions for uint8_t array to other data types / arrays:
	uint32_t GetUnsignedLong();
};



#endif /* INC_RASPBERRYPI_HPP_ */
