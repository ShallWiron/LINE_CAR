#include "oled.h"
#include "oled_font.h"
#include "delay.h"
#include <string.h>

#define OLED_I2C_ADDR    0x78
#define OLED_GPIO        GPIOB
#define OLED_SCL_PIN     GPIO_Pin_10
#define OLED_SDA_PIN     GPIO_Pin_11

/* Framebuffer 128x64 pixels (1024 bytes) */
static uint8_t s_oled_buffer[1024];

/* Macro dieu khien chan I2C */
#define I2C_SCL_H()   GPIO_SetBits(OLED_GPIO, OLED_SCL_PIN)
#define I2C_SCL_L()   GPIO_ResetBits(OLED_GPIO, OLED_SCL_PIN)
#define I2C_SDA_H()   GPIO_SetBits(OLED_GPIO, OLED_SDA_PIN)
#define I2C_SDA_L()   GPIO_ResetBits(OLED_GPIO, OLED_SDA_PIN)
#define I2C_SDA_READ() GPIO_ReadInputDataBit(OLED_GPIO, OLED_SDA_PIN)

static void I2C_Delay(void)
{
    for (volatile uint32_t i = 0; i < 4; i++)
    {
        __NOP();
    }
}

static void I2C_Start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    I2C_Delay();
    I2C_SDA_L();
    I2C_Delay();
    I2C_SCL_L();
}

static void I2C_Stop(void)
{
    I2C_SDA_L();
    I2C_SCL_H();
    I2C_Delay();
    I2C_SDA_H();
    I2C_Delay();
}

static void I2C_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (byte & 0x80)
        {
            I2C_SDA_H();
        }
        else
        {
            I2C_SDA_L();
        }
        I2C_Delay();
        I2C_SCL_H();
        I2C_Delay();
        I2C_SCL_L();
        byte <<= 1;
    }

    /* Wait ACK */
    I2C_SDA_H();
    I2C_Delay();
    I2C_SCL_H();
    I2C_Delay();
    I2C_SCL_L();
}

static void OLED_WriteCommand(uint8_t cmd)
{
    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WriteByte(0x00); /* Co = 0, D/C = 0 (Command) */
    I2C_WriteByte(cmd);
    I2C_Stop();
}

static void OLED_WriteDataBlock(const uint8_t *data, uint16_t length)
{
    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WriteByte(0x40); /* Co = 0, D/C = 1 (Data) */
    for (uint16_t i = 0; i < length; i++)
    {
        I2C_WriteByte(data[i]);
    }
    I2C_Stop();
}

void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Cap clock cho GPIOB */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* Cau hinh PB10 (SCL) va PB11 (SDA) lam Open-Drain Output */
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN | OLED_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_GPIO, &GPIO_InitStructure);

    I2C_SCL_H();
    I2C_SDA_H();
    delay_ms(50);

    /* Cac lenh khoi tao SSD1306 */
    OLED_WriteCommand(0xAE); // Tat man hinh
    OLED_WriteCommand(0xD5); // Set clock divide ratio/oscillator frequency
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8); // Set multiplex ratio (64)
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3); // Set display offset
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40); // Set start line = 0
    OLED_WriteCommand(0x8D); // Charge pump setting
    OLED_WriteCommand(0x14); // Bat charge pump
    OLED_WriteCommand(0x20); // Memory addressing mode
    OLED_WriteCommand(0x00); // 00 = Horizontal addressing mode
    OLED_WriteCommand(0xA1); // Column address 127 mapped to SEG0 (dao chieu ngang)
    OLED_WriteCommand(0xC8); // COM scan direction remapped (dao chieu doc)
    OLED_WriteCommand(0xDA); // Set COM pins hardware configuration
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81); // Set contrast control
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9); // Set pre-charge period
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB); // Set VCOMH deselect level
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA4); // Output follows RAM content
    OLED_WriteCommand(0xA6); // Normal display (khong dao nguoc)
    OLED_WriteCommand(0xAF); // Bat man hinh

    OLED_Clear();
    OLED_Update();
}

void OLED_Clear(void)
{
    memset(s_oled_buffer, 0, sizeof(s_oled_buffer));
}

void OLED_Update(void)
{
    OLED_WriteCommand(0x21); // Set column address
    OLED_WriteCommand(0);    // Start column
    OLED_WriteCommand(127);  // End column

    OLED_WriteCommand(0x22); // Set page address
    OLED_WriteCommand(0);    // Start page
    OLED_WriteCommand(7);    // End page (8 pages = 64 pixels)

    OLED_WriteDataBlock(s_oled_buffer, sizeof(s_oled_buffer));
}

void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    if (color == OLED_COLOR_WHITE)
    {
        s_oled_buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y % 8));
    }
    else
    {
        s_oled_buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
    }
}

void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
    int16_t dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    int8_t sx = (x0 < x1) ? 1 : -1;
    int8_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1)
    {
        OLED_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    OLED_DrawLine(x, y, x + w - 1, y, color);
    OLED_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    OLED_DrawLine(x, y, x, y + h - 1, color);
    OLED_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    for (uint8_t i = x; i < x + w && i < OLED_WIDTH; i++)
    {
        for (uint8_t j = y; j < y + h && j < OLED_HEIGHT; j++)
        {
            OLED_DrawPixel(i, j, color);
        }
    }
}

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color)
{
    if (ch < ' ' || ch > '~')
    {
        ch = ' ';
    }
    uint8_t c_idx = ch - ' ';

    if (size == 16)
    {
        /* Chan an toan tuyet doi de khong bao gio doc tran mang font */
        if (c_idx >= 95)
        {
            c_idx = 0;
        }

        /* Font 8x16 */
        for (uint8_t i = 0; i < 8; i++)
        {
            uint8_t line1 = OLED_F8x16[c_idx][i];
            uint8_t line2 = OLED_F8x16[c_idx][i + 8];

            for (uint8_t j = 0; j < 8; j++)
            {
                if (line1 & (1 << j))
                {
                    OLED_DrawPixel(x + i, y + j, color);
                }
                else
                {
                    OLED_DrawPixel(x + i, y + j, !color);
                }

                if (line2 & (1 << j))
                {
                    OLED_DrawPixel(x + i, y + j + 8, color);
                }
                else
                {
                    OLED_DrawPixel(x + i, y + j + 8, !color);
                }
            }
        }
    }
    else
    {
        /* Chan an toan cho font 6x8 */
        if (c_idx >= 95)
        {
            c_idx = 0;
        }

        /* Font 6x8 mac dinh */
        for (uint8_t i = 0; i < 6; i++)
        {
            uint8_t line = OLED_F6x8[c_idx][i];
            for (uint8_t j = 0; j < 8; j++)
            {
                if (line & (1 << j))
                {
                    OLED_DrawPixel(x + i, y + j, color);
                }
                else
                {
                    OLED_DrawPixel(x + i, y + j, !color);
                }
            }
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size, uint8_t color)
{
    uint8_t char_w = (size == 16) ? 8 : 6;
    while (*str)
    {
        /* Khong tu dong wrap xuong dong tiep theo de tranh de len cac dong ben duoi */
        if (x + char_w > OLED_WIDTH)
        {
            break;
        }
        if (y >= OLED_HEIGHT) break;
        OLED_ShowChar(x, y, *str, size, color);
        x += char_w;
        str++;
    }
}

static uint32_t Power10(uint8_t m)
{
    uint32_t result = 1;
    while (m--) result *= 10;
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    uint8_t char_w = (size == 16) ? 8 : 6;
    for (uint8_t t = 0; t < len; t++)
    {
        uint8_t temp = (num / Power10(len - t - 1)) % 10;
        OLED_ShowChar(x + t * char_w, y, temp + '0', size, color);
    }
}

void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    uint8_t char_w = (size == 16) ? 8 : 6;
    if (num < 0)
    {
        OLED_ShowChar(x, y, '-', size, color);
        num = -num;
    }
    else
    {
        OLED_ShowChar(x, y, '+', size, color);
    }
    OLED_ShowNum(x + char_w, y, (uint32_t)num, len, size, color);
}

void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t int_len, uint8_t frac_len, uint8_t size, uint8_t color)
{
    uint8_t char_w = (size == 16) ? 8 : 6;
    if (num < 0.0f)
    {
        OLED_ShowChar(x, y, '-', size, color);
        x += char_w;
        num = -num;
    }

    uint32_t int_part = (uint32_t)num;
    OLED_ShowNum(x, y, int_part, int_len, size, color);
    x += int_len * char_w;

    if (frac_len > 0)
    {
        OLED_ShowChar(x, y, '.', size, color);
        x += char_w;

        float frac_f = (num - (float)int_part) * (float)Power10(frac_len);
        uint32_t frac_part = (uint32_t)(frac_f + 0.5f);
        OLED_ShowNum(x, y, frac_part, frac_len, size, color);
    }
}

void OLED_DrawSensorBars(const uint16_t *sensors, int16_t line_pos)
{
    /* Khung vi tri do thi cam bien: tu y=50 toi y=63 (chieu cao 13 pixel) */
    const uint8_t base_y = 62;
    const uint8_t max_h = 10;
    const uint8_t bar_w = 10;
    const uint8_t bar_gap = 4;
    const uint8_t start_x = 9;

    /* Ve 8 cot tuong ung 8 cam bien QTR-8A */
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t x = start_x + i * (bar_w + bar_gap);
        uint8_t h = (uint8_t)((uint32_t)sensors[i] * max_h / 1000U);
        if (h > max_h) h = max_h;

        /* Ve khung vien ngoai cho moi cot */
        OLED_DrawRect(x, base_y - max_h, bar_w, max_h + 1, OLED_COLOR_WHITE);

        /* To day phan gia tri doc duoc */
        if (h > 0)
        {
            OLED_FillRect(x + 1, base_y - h + 1, bar_w - 2, h, OLED_COLOR_WHITE);
        }
    }

    /* Ve tam trung tam (Line mong doi = 3500) */
    uint8_t center_x = start_x + (8 * (bar_w + bar_gap) - bar_gap) / 2;
    OLED_DrawPixel(center_x, base_y - max_h - 2, OLED_COLOR_WHITE);

    /* Ve con tro vi tri duong line hien tai tren thanh do thi */
    if (line_pos >= 0 && line_pos <= 7000)
    {
        uint8_t total_w = 7 * (bar_w + bar_gap);
        uint8_t pos_x = start_x + bar_w / 2 + (uint8_t)((uint32_t)line_pos * total_w / 7000U);
        if (pos_x < OLED_WIDTH)
        {
            OLED_DrawPixel(pos_x, base_y - max_h - 2, OLED_COLOR_WHITE);
            OLED_DrawPixel(pos_x - 1, base_y - max_h - 3, OLED_COLOR_WHITE);
            OLED_DrawPixel(pos_x + 1, base_y - max_h - 3, OLED_COLOR_WHITE);
        }
    }
}

