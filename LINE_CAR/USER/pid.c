#include "pid.h"

void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd, int16_t base_speed, int16_t max_speed, int16_t min_speed)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->base_speed = base_speed;
    pid->max_speed = max_speed;
    pid->min_speed = min_speed;
    pid->integral_limit = 10000.0f;
    PID_Reset(pid);
}

void PID_Reset(PID_Controller_t *pid)
{
    pid->error = 0;
    pid->last_error = 0;
    pid->integral = 0.0f;
}

void PID_ResetIntegral(PID_Controller_t *pid)
{
    pid->integral = 0.0f;
}

void PID_SetTunings(PID_Controller_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void PID_SetSpeeds(PID_Controller_t *pid, int16_t base_speed, int16_t max_speed, int16_t min_speed)
{
    pid->base_speed = base_speed;
    pid->max_speed = max_speed;
    pid->min_speed = min_speed;
}

int16_t PID_Compute(PID_Controller_t *pid, int16_t current_position, int16_t target_position)
{
    /* Sai so vi tri: target = 3500 (trung tam cam bien) */
    pid->error = current_position - target_position;

    /* Thanh phan Ti Le (Proportional) */
    float p_term = pid->kp * (float)pid->error;

    /* Thanh phan Tich Phan (Integral) co chong bao hoa (Anti-windup) */
    pid->integral += (float)pid->error;
    if (pid->integral > pid->integral_limit)
    {
        pid->integral = pid->integral_limit;
    }
    else if (pid->integral < -pid->integral_limit)
    {
        pid->integral = -pid->integral_limit;
    }
    float i_term = pid->ki * pid->integral;

    /* Thanh phan Vi Phan (Derivative) */
    float d_term = pid->kd * (float)(pid->error - pid->last_error);
    pid->last_error = pid->error;

    float output = p_term + i_term + d_term;

    /* Gioi han tin hieu dau ra */
    if (output > 100.0f) output = 100.0f;
    if (output < -100.0f) output = -100.0f;

    return (int16_t)output;
}

void PID_CalculateMotorSpeeds(const PID_Controller_t *pid, int16_t pid_output, int16_t *speed_left, int16_t *speed_right)
{
    /* Giam toc dong khi vao cua (Dynamic Corner Deceleration):
     * Khi sai lech lon (|pid_output| > 35), giam toc do co so de tranh quan tinh vang xe */
    int16_t eff_base = pid->base_speed;
    int16_t abs_out = (pid_output < 0) ? -pid_output : pid_output;
    if (abs_out > 35)
    {
        int16_t decel = (int16_t)(((int32_t)(abs_out - 35) * pid->base_speed) / 100);
        eff_base -= decel;
        if (eff_base < 12)
        {
            eff_base = 12;
        }
    }

    /* Khi error > 0 (xe lech sang ben phai vach line):
     * Can quay sang trai -> Giam toc banh trai, tang toc banh phai */
    int16_t left = eff_base - pid_output;
    int16_t right = eff_base + pid_output;

    /* Gioi han toc do banh trai */
    if (left > pid->max_speed) left = pid->max_speed;
    if (left < pid->min_speed) left = pid->min_speed;

    /* Gioi han toc do banh phai */
    if (right > pid->max_speed) right = pid->max_speed;
    if (right < pid->min_speed) right = pid->min_speed;

    *speed_left = left;
    *speed_right = right;
}

