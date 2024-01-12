#ifndef RING_BUFFER_H__
#define RING_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define RING_BUFFER_LENGTH 1000


typedef struct {
	uint16_t start;
	uint16_t end;
} RingBufferLock;


typedef struct {
	float buf[RING_BUFFER_LENGTH];
	uint16_t head, tail;
	RingBufferLock lock;
	uint8_t lockCount;
} RingBuffer;

typedef enum {
	RING_BUFFER_OK = 0x0,
	RING_BUFFER_FULL,
	RING_BUFFER_NO_SUFFICIENT_SPACE
} RingBuffer_Status;

uint16_t RingBuffer_GetDataLength(RingBuffer *buf);
uint16_t RingBuffer_GetFreeSpace(RingBuffer *buf);
void RingBuffer_Init(RingBuffer *buf);
uint16_t RingBuffer_Read(RingBuffer *buf, uint8_t *data, uint16_t len);
uint8_t RingBuffer_Write(RingBuffer *buf, uint8_t *data, uint16_t len);
void RingBuffer_LockRegion(RingBuffer *buf, uint16_t start, uint16_t len);
void RingBuffer_UnlockRegion(RingBuffer *buf, uint16_t start, uint16_t len);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif //#ifndef RING_BUFFER_H__