#include "stm32f10x.h"
#include "car_app.h"

int main(void)
{
    /* 1. Khoi tao toan bo he thong: Delay, TB6612, QTR-8A, OLED, Button, PID */
    Car_Init();

    /* 2. Vong lap dieu khien chinh */
    while (1)
    {
        Car_Loop();
    }
}


