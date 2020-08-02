/*#ifndef SPI_H_
#define SPI_H_

// Raspberry Pi Communication

uint8_t Pi_ReadUInt8(SPI_HandleTypeDef* hspi);
uint16_t Pi_ReadUInt16(SPI_HandleTypeDef* hspi);
void Pi_ReadUInt16Chunk(SPI_HandleTypeDef* hspi, uint16_t* in, uint32_t size);
uint32_t Pi_ReadUInt32(SPI_HandleTypeDef* hspi);
void Pi_ReadUInt32Chunk(SPI_HandleTypeDef* hspi, uint32_t* in, uint32_t size);
float Pi_ReadFloat(SPI_HandleTypeDef* hspi);
void Pi_ReadFloatChunk(SPI_HandleTypeDef* hspi, float* in, uint32_t size);


void Pi_WriteUInt8(SPI_HandleTypeDef* hspi, uint8_t out);
void Pi_WriteUInt16(SPI_HandleTypeDef* hspi, uint16_t out);
void Pi_WriteUInt16Chunk(SPI_HandleTypeDef* hspi, uint16_t* out, uint32_t size);
void Pi_WriteUInt32(SPI_HandleTypeDef* hspi, uint32_t out);
void Pi_WriteUInt32Chunk(SPI_HandleTypeDef* hspi, uint32_t* out, uint32_t size);
void Pi_WriteFloat(SPI_HandleTypeDef* hspi, float out);
void Pi_WriteFloatChunk(SPI_HandleTypeDef* hspi, float* out, uint32_t size);




class SPI_Dev
{
public:
	void Callback() { };
};


#endif
*/
