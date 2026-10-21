#ifndef __BUTTON_H
#define __BUTTON_H

#include "stm32f10x.h"
#include <stdint.h>

/*
 * Dinh nghia 4 nut bam tren xe:
 *  - BTN_RUN  : PB3 (Nut 1 - Chay / Dung khanc cap / Thoat Menu)
 *  - BTN_MENU : PB4 (Nut 2 - Vao Menu / Giam gia tri - )
 *  - BTN_UP   : PB5 (Nut 3 - Tang gia tri + / Calib nhanh)
 *  - BTN_DOWN : PB6 (Nut 4 - Di chuyen xuong v / Chon che do)
 */
typedef enum {
    BTN_RUN   = 0,  /* PB3 */
    BTN_MENU  = 1,  /* PB4 */
    BTN_UP    = 2,  /* PB5 */
    BTN_DOWN  = 3,  /* PB6 */
    BTN_COUNT = 4
} Button_ID_t;

/* Alias tuong thich nguoc */
#define BTN_MODE BTN_MENU

void Button_Init(void);
void Button_Update(void);

/* Tra ve 1 neu co su kien nhan (hoac auto-repeat khi de giu) */
uint8_t Button_WasPressed(Button_ID_t btn);

/* Tra ve trang thai logic hien tai (1 = dang de giu, 0 = nha) */
uint8_t Button_IsDown(Button_ID_t btn);

#endif /* __BUTTON_H */
