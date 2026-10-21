#include "tb6612.h"

#define AIN1_PIN    GPIO_Pin_13
#define AIN2_PIN    GPIO_Pin_12
#define BIN1_PIN    GPIO_Pin_14
#define BIN2_PIN    GPIO_Pin_15
#define MOTOR_GPIO  GPIOB

#define PWM_A_PIN   GPIO_Pin_9
#define PWM_B_PIN   GPIO_Pin_8
#define PWM_GPIO    GPIOA

#define PWM_PERIOD  999U

static uint8_t s_invert_a = 0;
static uint8_t s_invert_b = 0;

static int16_t LimitSpeed(int16_t speed)
{
    if (speed > 100)  speed = 100;
    if (speed < -100) speed = -100;
    return speed;
}

void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 1. Cap clock GPIOA, GPIOB, AFIO va TIM1 */
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_GPIOB |
        RCC_APB2Periph_AFIO  |
        RCC_APB2Periph_TIM1,
        ENABLE
    );

    /* 2. Cau hinh PB12-PB15 lam Output Push-Pull */
    GPIO_InitStructure.GPIO_Pin = AIN1_PIN | AIN2_PIN | BIN1_PIN | BIN2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_GPIO, &GPIO_InitStructure);

    GPIO_ResetBits(MOTOR_GPIO, AIN1_PIN | AIN2_PIN | BIN1_PIN | BIN2_PIN);

    /* 3. Cau hinh PA8 (PWMB) va PA9 (PWMA) lam Alternate Function Push-Pull */
    GPIO_InitStructure.GPIO_Pin = PWM_A_PIN | PWM_B_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PWM_GPIO, &GPIO_InitStructure);

    /* 4. Cau hinh Timer TIM1 (72MHz / (71 + 1) / (999 + 1) = 1kHz) */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* 5. Cau hinh PWM mode 1 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    /* PA8 = TIM1_CH1 = PWMB (Motor B) */
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* PA9 = TIM1_CH2 = PWMA (Motor A) */
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* 6. Bat preload va main output TIM1 */
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    /* 7. Bat timer */
    TIM_Cmd(TIM1, ENABLE);
}

void Motor_SetInvert(uint8_t invertA, uint8_t invertB)
{
    s_invert_a = invertA ? 1 : 0;
    s_invert_b = invertB ? 1 : 0;
}

/* Dieu khien dong co A (Trai hoac Phai tuy lap rap) */
void Motor_A_SetSpeed(int16_t speed)
{
    speed = LimitSpeed(speed);
    if (s_invert_a)
    {
        speed = -speed;
    }

    if (speed > 0)
    {
        GPIO_SetBits(MOTOR_GPIO, AIN1_PIN);
        GPIO_ResetBits(MOTOR_GPIO, AIN2_PIN);
    }
    else if (speed < 0)
    {
        GPIO_ResetBits(MOTOR_GPIO, AIN1_PIN);
        GPIO_SetBits(MOTOR_GPIO, AIN2_PIN);
        speed = -speed;
    }
    else
    {
        GPIO_ResetBits(MOTOR_GPIO, AIN1_PIN | AIN2_PIN);
    }

    TIM_SetCompare2(TIM1, ((uint32_t)speed * PWM_PERIOD) / 100U);
}

/* Dieu khien dong co B (Trai hoac Phai tuy lap rap) */
void Motor_B_SetSpeed(int16_t speed)
{
    speed = LimitSpeed(speed);
    if (s_invert_b)
    {
        speed = -speed;
    }

    if (speed > 0)
    {
        GPIO_SetBits(MOTOR_GPIO, BIN1_PIN);
        GPIO_ResetBits(MOTOR_GPIO, BIN2_PIN);
    }
    else if (speed < 0)
    {
        GPIO_ResetBits(MOTOR_GPIO, BIN1_PIN);
        GPIO_SetBits(MOTOR_GPIO, BIN2_PIN);
        speed = -speed;
    }
    else
    {
        GPIO_ResetBits(MOTOR_GPIO, BIN1_PIN | BIN2_PIN);
    }

    TIM_SetCompare1(TIM1, ((uint32_t)speed * PWM_PERIOD) / 100U);
}

void Motor_SetSpeed(int16_t speedA, int16_t speedB)
{
    Motor_A_SetSpeed(speedA);
    Motor_B_SetSpeed(speedB);
}

void Motor_Stop(void)
{
    Motor_A_SetSpeed(0);
    Motor_B_SetSpeed(0);
}

void Motor_Brake(void)
{
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    GPIO_SetBits(MOTOR_GPIO, AIN1_PIN | AIN2_PIN | BIN1_PIN | BIN2_PIN);
}
