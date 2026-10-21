#ifndef __CAR_APP_H
#define __CAR_APP_H

#include "stm32f10x.h"
#include <stdint.h>
#include "pid.h"
#include "qtr8a.h"

typedef enum {
    CAR_STATE_STOPPED = 0,
    CAR_STATE_RUNNING,
    CAR_STATE_CALIBRATING,
    CAR_STATE_MENU
} CarState_t;

void Car_Init(void);
void Car_Loop(void);

#endif /* __CAR_APP_H */

