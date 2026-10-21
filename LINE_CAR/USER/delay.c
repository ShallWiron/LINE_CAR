#include "delay.h"

static volatile uint32_t s_millis_count = 0;

void Delay_Init(void)
{
    /* Cau hinh SysTick ngat moi 1ms */
    SysTick_Config(SystemCoreClock / 1000U);
}

void SysTick_Handler(void)
{
    s_millis_count++;
}

uint32_t millis(void)
{
    return s_millis_count;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = s_millis_count;
    while ((s_millis_count - start) < ms)
    {
        __NOP();
    }
}

void delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    uint32_t t_old = SysTick->VAL;
    uint32_t t_now;
    uint32_t t_cnt = 0;
    uint32_t reload = SysTick->LOAD;

    while (t_cnt < ticks)
    {
        t_now = SysTick->VAL;
        if (t_now != t_old)
        {
            if (t_now < t_old)
            {
                t_cnt += t_old - t_now;
            }
            else
            {
                t_cnt += reload - t_now + t_old;
            }
            t_old = t_now;
        }
    }
}

