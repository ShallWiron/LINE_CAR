#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"
#include <stdint.h>

#define OLED_WIDTH   128
#define OLED_HEIGHT  64

#define OLED_COLOR_BLACK  0
#define OLED_COLOR_WHITE  1

/* Khoi tao man hinh OLED I2C tren PB10 (SCL), PB11 (SDA) */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);

void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size, uint8_t color);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t int_len, uint8_t frac_len, uint8_t size, uint8_t color);

/* Ve thanh do thi 8 cam bien QTR-8A va con tro vi tri line */
void OLED_DrawSensorBars(const uint16_t *sensors, int16_t line_pos);

#endif /* __OLED_H */

