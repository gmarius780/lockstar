#define SPI_MASTER SPI4 // SPI2, SPI4
// #define SPI_SLAVE SPI2  // SPI2, SPI4

#define LOOPBACK_MODE // enable loopback mode
// #define MASTERTX_SLAVERX_TEST // enable Master Tx Slave Rx test

#define BAUDRATE_PRESCALER                                                     \
  LL_SPI_BAUDRATEPRESCALER_DIV2 // DIV2, DIV4, DIV8, DIV16, DIV32, DIV64,
                                // DIV128, DIV256
#define SPI_MASTER_DIRECTION                                                   \
  LL_SPI_FULL_DUPLEX // FULL_DUPLEX, HALF_DUPLEX_TX, HALF_DUPLEX_RX, SIMPLEX_TX,
                     // SIMPLEX_RX
#define SPI_SLAVE_DIRECTION                                                    \
  LL_SPI_SIMPLEX_RX // FULL_DUPLEX, HALF_DUPLEX_TX, HALF_DUPLEX_RX, SIMPLEX_TX,
                    // SIMPLEX_RX

#define NSS_MODE_MASTER                                                        \
  LL_SPI_NSS_HARD_OUTPUT // LL_SPI_NSS_SOFT, LL_SPI_NSS_HARD_INPUT,
                         // LL_SPI_NSS_HARD_OUTPUT
#define NSS_MODE_SLAVE                                                         \
  LL_SPI_NSS_HARD_INPUT // LL_SPI_NSS_SOFT, LL_SPI_NSS_HARD_INPUT,
                        // LL_SPI_NSS_HARD_OUTPUT
#define BUFFER_SIZE 1080
#define SEED 4

// SPI2 TX: DMA1_Stream1, RX: DMA1_Stream0
#define SPI2_TXDMA DMA1
#define SPI2_TXDMA_STREAM LL_DMA_STREAM_3
#define SPI2_RXDMA DMA1
#define SPI2_RXDMA_STREAM LL_DMA_STREAM_2

// SPI4 TX: DMA1_Stream6, RX: DMA1_Stream5
#define SPI4_TXDMA DMA2
#define SPI4_TXDMA_STREAM LL_DMA_STREAM_1
#define SPI4_RXDMA DMA2
#define SPI4_RXDMA_STREAM LL_DMA_STREAM_0

#define SPI_MASTER_TXDMA SPI4_TXDMA
#define SPI_MASTER_TXDMA_STREAM SPI4_TXDMA_STREAM
#define SPI_MASTER_RXDMA SPI4_RXDMA
#define SPI_MASTER_RXDMA_STREAM SPI4_RXDMA_STREAM

// #define SPI_MASTER_TXDMA SPI2_TXDMA
// #define SPI_MASTER_TXDMA_STREAM SPI2_TXDMA_STREAM
// #define SPI_MASTER_RXDMA SPI2_RXDMA
// #define SPI_MASTER_RXDMA_STREAM SPI2_RXDMA_STREAM

#define DMA_MODE
// #define PACKET_MODE

#define USE_FIFOS