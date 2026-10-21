#ifndef __TB6612_H
#define __TB6612_H

#include "stm32f10x.h"
#include <stdint.h>

/*
 * TB6612FNG Driver Pinout (Theo so do mach):
 *  - PWMA: PA9  (TIM1_CH2)
 *  - PWMB: PA8  (TIM1_CH1)
 *  - AIN1: PB13
 *  - AIN2: PB12
 *  - BIN1: PB14
 *  - BIN2: PB15
 *  - STBY: Noi VCC tren mach phan cung
 */

void Motor_Init(void);
void Motor_SetInvert(uint8_t invertA, uint8_t invertB);
void Motor_A_SetSpeed(int16_t speed);
void Motor_B_SetSpeed(int16_t speed);
void Motor_SetSpeed(int16_t speedA, int16_t speedB);
void Motor_Stop(void);
void Motor_Brake(void);

#endif /* __TB6612_H */
