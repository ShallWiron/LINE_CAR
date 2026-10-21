#include "button.h"
#include "delay.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t state;            /* 1 = dang nhan, 0 = nha */
    uint8_t debounce_count;   /* Dem mau chong rung */
    uint8_t was_pressed;      /* Co bao su kien nhan */
    uint32_t press_time;      /* Thoi diem bat dau nhan */
    uint32_t repeat_time;     /* Thoi diem auto-repeat */
} Button_State_t;

static Button_State_t s_buttons[BTN_COUNT] = {
    {GPIOB, GPIO_Pin_3, 0, 0, 0, 0, 0}, /* PB3 - BTN 1 */
    {GPIOB, GPIO_Pin_4, 0, 0, 0, 0, 0}, /* PB4 - BTN 2 */
    {GPIOB, GPIO_Pin_5, 0, 0, 0, 0, 0}, /* PB5 - BTN 3 */
    {GPIOB, GPIO_Pin_6, 0, 0, 0, 0, 0}  /* PB6 - BTN 4 */
};

void Button_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. Cap clock GPIOB va khoi AFIO */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* 2. VO HIEU HOA JTAG de giai phong PB3 (JTDO) va PB4 (JNTRST) lam GPIO thuong
     * Dong thoi GIU NGUYEN SW-DP (PA13/PA14) de nap code va debug binh thuong */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* 3. Cau hinh PB3, PB4, PB5, PB6 lam Input Pull-Up (IPU) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 4. Khoi tao lai cac bien trang thai */
    for (uint8_t i = 0; i < BTN_COUNT; i++)
    {
        s_buttons[i].state = 0;
        s_buttons[i].debounce_count = 0;
        s_buttons[i].was_pressed = 0;
        s_buttons[i].press_time = 0;
        s_buttons[i].repeat_time = 0;
    }
}

void Button_Update(void)
{
    static uint32_t last_update = 0;
    uint32_t now = millis();

    /* Quet phim moi 10ms */
    if (now - last_update < 10)
    {
        return;
    }
    last_update = now;

    for (uint8_t i = 0; i < BTN_COUNT; i++)
    {
        /* Nut bam noi GND: Khi nhan xuong pin ve muc LOW (Bit_RESET) */
        uint8_t is_down = (GPIO_ReadInputDataBit(s_buttons[i].port, s_buttons[i].pin) == Bit_RESET) ? 1 : 0;

        if (is_down)
        {
            if (s_buttons[i].debounce_count < 2)
            {
                s_buttons[i].debounce_count++;
                if (s_buttons[i].debounce_count == 2) /* 20ms la kich hoat ngay lap tuc! */
                {
                    s_buttons[i].state = 1;
                    s_buttons[i].was_pressed = 1;
                    s_buttons[i].press_time = now;
                }
            }
            else
            {
                /* Giu nut tren 400ms thi bat dau auto-repeat (tien loi khi chinh thong so) */
                if (now - s_buttons[i].press_time > 400)
                {
                    if (now - s_buttons[i].repeat_time > 150)
                    {
                        s_buttons[i].repeat_time = now;
                        s_buttons[i].was_pressed = 1;
                    }
                }
                else
                {
                    s_buttons[i].repeat_time = now;
                }
            }
        }
        else
        {
            /* Khi nha nut: Reset trang thai tuc thi */
            s_buttons[i].debounce_count = 0;
            s_buttons[i].state = 0;
        }
    }
}

uint8_t Button_WasPressed(Button_ID_t btn)
{
    if (btn >= BTN_COUNT) return 0;
    if (s_buttons[btn].was_pressed)
    {
        s_buttons[btn].was_pressed = 0;
        return 1;
    }
    return 0;
}

uint8_t Button_IsDown(Button_ID_t btn)
{
    if (btn >= BTN_COUNT) return 0;
    return s_buttons[btn].state;
}
