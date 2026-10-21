#ifndef __PID_H
#define __PID_H

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;

    int16_t error;
    int16_t last_error;
    float integral;
    float integral_limit;

    int16_t base_speed;
    int16_t max_speed;
    int16_t min_speed;      /* Co the am de quay tai cho (vi du: -55) */
} PID_Controller_t;

void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd, int16_t base_speed, int16_t max_speed, int16_t min_speed);
void PID_Reset(PID_Controller_t *pid);
void PID_ResetIntegral(PID_Controller_t *pid);
void PID_SetTunings(PID_Controller_t *pid, float kp, float ki, float kd);
void PID_SetSpeeds(PID_Controller_t *pid, int16_t base_speed, int16_t max_speed, int16_t min_speed);

/* Tinh toan tin hieu dieu khien PID tu sai lech vi tri */
int16_t PID_Compute(PID_Controller_t *pid, int16_t current_position, int16_t target_position);

/* Tinh toan toc do 2 banh xe tu gia tri dieu khien (kem giam toc vao cua dong) */
void PID_CalculateMotorSpeeds(const PID_Controller_t *pid, int16_t pid_output, int16_t *speed_left, int16_t *speed_right);

#endif /* __PID_H */

