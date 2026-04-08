#ifndef __USART1_DMA_H
#define __USART1_DMA_H

#include <stddef.h>
#include <stdint.h>

#ifndef USART1_DMA_TX_BUFFER_SIZE
#define USART1_DMA_TX_BUFFER_SIZE 512U
#endif

/*
 * USART1 TX DMA printf driver.
 *
 * Hardware:
 *   USART1_TX -> PA9
 *   DMA1_Channel4 -> USART1_TX
 *
 * Usage:
 *   1. Call usart1_dma_init(115200) once during startup.
 *   2. Use printf(), usart1_dma_printf(), or usart1_dma_write().
 *   3. Keep DMA1_Channel4_IRQHandler connected to usart1_dma_irq_handler().
 *
 * Behavior:
 *   Writes copy data into a ring buffer and return immediately.
 *   If the buffer is full, the remaining bytes are dropped instead of blocking.
 */

/* Initialize USART1 TX on PA9 and DMA1 Channel4. */
void usart1_dma_init(uint32_t baudrate);

/* Queue raw bytes for DMA transmission. Returns bytes accepted into the buffer. */
size_t usart1_dma_write(const uint8_t *data, size_t length);

/* Format and queue a short message. The internal format buffer is 128 bytes. */
int usart1_dma_printf(const char *format, ...);

/* Return the total number of bytes dropped because the TX buffer was full. */
uint32_t usart1_dma_dropped_bytes(void);

/* Call from DMA1_Channel4_IRQHandler. */
void usart1_dma_irq_handler(void);

#endif
