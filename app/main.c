#include "stm32f10x.h"

static void gpio_init(void);
static void delay(volatile uint32_t count);

int main(void)
{
    gpio_init();

    while (1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        delay(800000);
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        delay(800000);
    }
}

static void gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &gpio);

    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

static void delay(volatile uint32_t count)
{
    while (count-- > 0U) {
        __NOP();
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) {
    }
}
#endif
