#include "usart1_dma.h"

#include "stm32f10x.h"

#include <stdarg.h>
#include <stdio.h>

#if USART1_DMA_TX_BUFFER_SIZE < 2U
#error "USART1_DMA_TX_BUFFER_SIZE must be at least 2"
#endif

static uint8_t tx_buffer[USART1_DMA_TX_BUFFER_SIZE];
static volatile uint16_t tx_head;
static volatile uint16_t tx_tail;
static volatile uint16_t tx_dma_length;
static volatile uint8_t tx_dma_busy;
static volatile uint8_t tx_initialized;
static volatile uint32_t tx_dropped_bytes;

static void usart1_dma_gpio_init(void);
static void usart1_dma_periph_init(uint32_t baudrate);
static void usart1_dma_nvic_init(void);
static void usart1_dma_start_locked(void);
static uint32_t irq_save(void);
static void irq_restore(uint32_t primask);

void usart1_dma_init(uint32_t baudrate)
{
    uint32_t primask = irq_save();

    tx_head = 0U;
    tx_tail = 0U;
    tx_dma_length = 0U;
    tx_dma_busy = 0U;
    tx_initialized = 0U;
    tx_dropped_bytes = 0U;

    irq_restore(primask);

    usart1_dma_gpio_init();
    usart1_dma_periph_init(baudrate);
    usart1_dma_nvic_init();

    primask = irq_save();
    tx_initialized = 1U;
    irq_restore(primask);
}

size_t usart1_dma_write(const uint8_t *data, size_t length)
{
    size_t written = 0U;
    uint32_t primask;

    if ((data == NULL) || (length == 0U)) {
        return 0U;
    }

    primask = irq_save();

    if (tx_initialized == 0U) {
        irq_restore(primask);
        return 0U;
    }

    while (written < length) {
        uint16_t next_head = (uint16_t)(tx_head + 1U);

        if (next_head >= USART1_DMA_TX_BUFFER_SIZE) {
            next_head = 0U;
        }

        if (next_head == tx_tail) {
            tx_dropped_bytes += (uint32_t)(length - written);
            break;
        }

        tx_buffer[tx_head] = data[written];
        tx_head = next_head;
        written++;
    }

    usart1_dma_start_locked();
    irq_restore(primask);

    return written;
}

int usart1_dma_printf(const char *format, ...)
{
    char buffer[128];
    va_list args;
    int length;

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length <= 0) {
        return length;
    }

    if ((size_t)length >= sizeof(buffer)) {
        length = (int)sizeof(buffer) - 1;
    }

    return (int)usart1_dma_write((const uint8_t *)buffer, (size_t)length);
}

uint32_t usart1_dma_dropped_bytes(void)
{
    uint32_t dropped;
    uint32_t primask = irq_save();

    dropped = tx_dropped_bytes;

    irq_restore(primask);

    return dropped;
}

void usart1_dma_irq_handler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC4) != RESET) {
        DMA_Cmd(DMA1_Channel4, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_TC4);

        tx_tail = (uint16_t)(tx_tail + tx_dma_length);
        if (tx_tail >= USART1_DMA_TX_BUFFER_SIZE) {
            tx_tail = (uint16_t)(tx_tail - USART1_DMA_TX_BUFFER_SIZE);
        }

        tx_dma_length = 0U;
        tx_dma_busy = 0U;
        usart1_dma_start_locked();
    }

    if (DMA_GetITStatus(DMA1_IT_TE4) != RESET) {
        DMA_Cmd(DMA1_Channel4, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_TE4);

        tx_tail = (uint16_t)(tx_tail + tx_dma_length);
        if (tx_tail >= USART1_DMA_TX_BUFFER_SIZE) {
            tx_tail = (uint16_t)(tx_tail - USART1_DMA_TX_BUFFER_SIZE);
        }

        tx_dropped_bytes += tx_dma_length;
        tx_dma_length = 0U;
        tx_dma_busy = 0U;
        usart1_dma_start_locked();
    }
}

int _write(int file, char *ptr, int len)
{
    (void)file;

    if (len <= 0) {
        return 0;
    }

    return (int)usart1_dma_write((const uint8_t *)ptr, (size_t)len);
}

static void usart1_dma_gpio_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
}

static void usart1_dma_periph_init(uint32_t baudrate)
{
    USART_InitTypeDef usart;
    DMA_InitTypeDef dma;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    USART_Cmd(USART1, DISABLE);
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_DeInit(DMA1_Channel4);

    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);

    dma.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)tx_buffer;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = 1U;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_Medium;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &dma);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC | DMA_IT_TE, ENABLE);

    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

static void usart1_dma_nvic_init(void)
{
    NVIC_InitTypeDef nvic;

    nvic.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

static void usart1_dma_start_locked(void)
{
    uint16_t length;

    if ((tx_dma_busy != 0U) || (tx_head == tx_tail)) {
        return;
    }

    if (tx_head > tx_tail) {
        length = (uint16_t)(tx_head - tx_tail);
    } else {
        length = (uint16_t)(USART1_DMA_TX_BUFFER_SIZE - tx_tail);
    }

    tx_dma_length = length;
    tx_dma_busy = 1U;

    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_ClearITPendingBit(DMA1_IT_GL4);
    DMA1_Channel4->CMAR = (uint32_t)&tx_buffer[tx_tail];
    DMA1_Channel4->CNDTR = length;
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

static uint32_t irq_save(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();

    return primask;
}

static void irq_restore(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}
