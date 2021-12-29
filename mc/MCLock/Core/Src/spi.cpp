/*#include "stm32f4xx_hal.h"
#include <cstdlib>
#include <algorithm>

uint8_t dummy_buffer[4];

#define TIMEOUT	1000000
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })




uint8_t Pi_ReadUInt8(SPI_HandleTypeDef* hspi)
{
	uint8_t buffer[1];
	HAL_SPI_TransmitReceive(hspi, dummy_buffer, buffer, 1, TIMEOUT);
	return *buffer;
}


uint16_t Pi_ReadUInt16(SPI_HandleTypeDef* hspi)
{
	volatile uint16_t buffer[1];
	HAL_SPI_TransmitReceive(hspi, dummy_buffer, (uint8_t*)buffer, 2, TIMEOUT);
	return *(buffer);
}


void Pi_ReadUInt16Chunk(SPI_HandleTypeDef* hspi, uint16_t* in, uint32_t size)
{
	uint32_t ints_left = size;
	uint32_t maxint = 0x1000/sizeof(uint16_t);
	for(int i =0;;i++){
		uint32_t s = min(maxint,ints_left);
		uint8_t* temp_buffer = (uint8_t*)malloc(size*sizeof(float));//new uint8_t[4*size];
		HAL_SPI_TransmitReceive(hspi, temp_buffer, (uint8_t*)(in+i*maxint), s*sizeof(uint16_t), TIMEOUT);
		free(temp_buffer);
		ints_left-=maxint;
		if (s<maxint)
			break;
	}
}


uint32_t Pi_ReadUInt32(SPI_HandleTypeDef* hspi)
{
	uint8_t buffer[4];
	HAL_SPI_TransmitReceive(hspi, dummy_buffer, buffer, 4, TIMEOUT);
	return *((uint32_t *)buffer);
}

void Pi_ReadUInt32Chunk(SPI_HandleTypeDef* hspi, uint32_t* in, uint32_t size)
{
	uint32_t ints_left = size;
	uint32_t maxint = 0x1000/sizeof(uint32_t);
	for(int i =0;;i++){
		uint32_t s = min(maxint,ints_left);
		uint8_t* temp_buffer = (uint8_t*)malloc(size*sizeof(float));//new uint8_t[4*size];
		HAL_SPI_TransmitReceive(hspi, temp_buffer, (uint8_t*)(in+i*maxint), s*sizeof(uint32_t), TIMEOUT);
		free(temp_buffer);
		ints_left-=maxint;
		if (s<maxint)
			break;
	}
}


float Pi_ReadFloat(SPI_HandleTypeDef* hspi)
{
	uint8_t buffer[4];
	HAL_SPI_TransmitReceive(hspi, dummy_buffer, buffer, 4, TIMEOUT);
	return *((float *)buffer);
}

void Pi_ReadFloatChunk(SPI_HandleTypeDef* hspi, float* in, uint32_t size)
{
	uint32_t floats_left = size;
	uint32_t maxfloat = 0x1000/sizeof(float);
	for(int i =0;;i++){
		uint32_t s = min(maxfloat,floats_left);
		uint8_t* temp_buffer = (uint8_t*)malloc(size*sizeof(float));//new uint8_t[4*size];
		HAL_SPI_TransmitReceive(hspi, temp_buffer, (uint8_t*)(in+i*1024), 4*s, TIMEOUT);
		free(temp_buffer);
		floats_left-=maxfloat;
		if (s<maxfloat)
			break;
	}
}






void Pi_WriteUInt8(SPI_HandleTypeDef* hspi, uint8_t out)
{
	HAL_SPI_TransmitReceive(hspi, &out, dummy_buffer, 1, TIMEOUT);
}


void Pi_WriteUInt16(SPI_HandleTypeDef* hspi, uint16_t out)
{
	uint8_t buffer[2];
	*((uint16_t *)buffer) = out;
	HAL_SPI_TransmitReceive(hspi, buffer, dummy_buffer, 2, TIMEOUT);
}


void Pi_WriteUInt16Chunk(SPI_HandleTypeDef* hspi, uint16_t* out, uint32_t size)
{
	uint32_t ints_left = size;
	uint32_t maxint = 0x1000/sizeof(uint16_t);
	for(int i =0;;i++){
		uint32_t s = min(maxint,ints_left);
		uint8_t* temp_buffer = (uint8_t*)malloc(s*sizeof(uint16_t));//new uint8_t[4*size];
		HAL_SPI_TransmitReceive(hspi, (uint8_t*)(out+i*maxint), temp_buffer, s*sizeof(uint16_t), TIMEOUT);
		free(temp_buffer);
		ints_left-=maxint;
		if (s<maxint)
			break;
	}
}


void Pi_WriteUInt32(SPI_HandleTypeDef* hspi, uint32_t out)
{
	uint8_t buffer[4];
	*((uint32_t *)buffer) = out;
	HAL_SPI_TransmitReceive(hspi, buffer, dummy_buffer, 4, TIMEOUT);
}


void Pi_WriteUInt32Chunk(SPI_HandleTypeDef* hspi, uint32_t* out, uint32_t size)
{
	uint32_t ints_left = size;
	uint32_t maxint = 0x1000/sizeof(uint32_t);
	for(int i =0;;i++){
		uint32_t s = min(maxint,ints_left);
		uint8_t* temp_buffer = (uint8_t*)malloc(s*sizeof(uint32_t));//new uint8_t[4*size];
		HAL_SPI_TransmitReceive(hspi, (uint8_t*)(out+i*maxint), temp_buffer, s*sizeof(uint32_t), TIMEOUT);
		free(temp_buffer);
		ints_left-=maxint;
		if (s<maxint)
			break;
	}
}


void Pi_WriteFloat(SPI_HandleTypeDef* hspi, float out)
{
	uint8_t buffer[4];
	*((float *)buffer) = out;
	HAL_SPI_TransmitReceive(hspi, buffer, dummy_buffer, 4, TIMEOUT);
	//HAL_SPI_TransmitReceive_DMA(hspi, buffer, dummy_buffer, 4);
}


void Pi_WriteFloatChunk(SPI_HandleTypeDef* hspi, float* out, uint32_t size)
{
	// size = amount of floats not bytes
	uint32_t floats_left = size;
	uint32_t maxfloat = 0x1000/sizeof(float);
	for(uint32_t i =0;;i++){
		uint32_t s = min(maxfloat,floats_left);
		uint8_t* temp_buffer = (uint8_t*)malloc(s*sizeof(float));//new uint8_t[4*size];
		HAL_SPI_TransmitReceive(hspi, (uint8_t*)(out+i*1024), temp_buffer, 4*s, TIMEOUT);
		//HAL_SPI_TransmitReceive_DMA(hspi, (uint8_t*)(out+i*1024), temp_buffer, 4*s);
		free(temp_buffer);
		floats_left-=maxfloat;
		if (s<maxfloat)
			break;
	}
}

*/
