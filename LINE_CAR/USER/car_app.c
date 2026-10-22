#include "car_app.h"
#include "delay.h"
#include "tb6612.h"
#include "oled.h"
#include "button.h"
#include "stm32f10x_flash.h"
#include <stdio.h>

#define FLASH_CONFIG_PAGE_ADDR   0x0800FC00U  /* Trang 63 (Trang cuoi Flash 64KB cua STM32F103C8) */
#define FLASH_CONFIG_MAGIC       0x55AA1234U

typedef struct {
    uint32_t magic;
    float kp;
    float ki;
    float kd;
    int16_t base_speed;
    int16_t max_speed;
    int16_t min_speed;
    uint8_t line_mode;
    uint8_t corner_advance_ms;
    uint16_t sensor_min[QTR8A_NUM_SENSORS];
    uint16_t sensor_max[QTR8A_NUM_SENSORS];
    uint32_t checksum;
} CarConfig_t;

static CarState_t s_state = CAR_STATE_STOPPED;
static PID_Controller_t s_pid;
static uint8_t s_line_mode = QTR8A_LINE_BLACK_WHITE;
static uint16_t s_sensors[QTR8A_NUM_SENSORS];
static int16_t s_line_pos = 3500;
static int16_t s_speed_left = 0;
static int16_t s_speed_right = 0;
static int16_t s_pid_out = 0;

/* May trang thai xu ly goc cua 90 do */
typedef enum {
    CORNER_MANEUVER_IDLE = 0,
    CORNER_MANEUVER_ADVANCE,  /* Truon toi: banh xe tien them 1 doan de truc banh toi dinh goc */
    CORNER_MANEUVER_BRAKE,    /* Phanh gap triet tieu quan tinh tien */
    CORNER_MANEUVER_PIVOT     /* Quay tai cho quanh truc banh xe */
} CornerManeuver_t;

static CornerManeuver_t s_corner_maneuver = CORNER_MANEUVER_IDLE;
static CornerType_t s_active_corner = CORNER_NONE;
static uint32_t s_corner_step_tick = 0;
static uint8_t s_corner_advance_ms = 70; /* Thoi gian truon mac dinh 70ms */

/* Menu tap trung toan bo 9 muc cai dat */
static uint8_t s_menu_item = 0;
#define MENU_TOTAL_ITEMS 9

static uint32_t Config_ComputeChecksum(const CarConfig_t *cfg)
{
    const uint8_t *p = (const uint8_t *)cfg;
    uint32_t sum = 0;
    for (uint32_t i = 0; i < sizeof(CarConfig_t) - sizeof(uint32_t); i++)
    {
        sum += p[i];
    }
    return sum;
}

static uint8_t Config_SaveToFlash(void)
{
    CarConfig_t cfg;
    cfg.magic = FLASH_CONFIG_MAGIC;
    cfg.kp = s_pid.kp;
    cfg.ki = s_pid.ki;
    cfg.kd = s_pid.kd;
    cfg.base_speed = s_pid.base_speed;
    cfg.max_speed = s_pid.max_speed;
    cfg.min_speed = s_pid.min_speed;
    cfg.line_mode = s_line_mode;
    cfg.corner_advance_ms = s_corner_advance_ms;

    const uint16_t *mins = QTR8A_GetMinValues();
    const uint16_t *maxs = QTR8A_GetMaxValues();
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        cfg.sensor_min[i] = mins[i];
        cfg.sensor_max[i] = maxs[i];
    }
    cfg.checksum = Config_ComputeChecksum(&cfg);

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    FLASH_Status status = FLASH_ErasePage(FLASH_CONFIG_PAGE_ADDR);
    if (status != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return 0;
    }

    uint32_t *p_data = (uint32_t *)&cfg;
    uint32_t words_count = sizeof(CarConfig_t) / 4;

    for (uint32_t i = 0; i < words_count; i++)
    {
        status = FLASH_ProgramWord(FLASH_CONFIG_PAGE_ADDR + (i * 4), p_data[i]);
        if (status != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return 0;
        }
    }

    FLASH_Lock();
    return 1;
}

static uint8_t Config_LoadFromFlash(void)
{
    CarConfig_t cfg;
    const uint32_t *p_flash = (const uint32_t *)FLASH_CONFIG_PAGE_ADDR;
    uint32_t *p_dest = (uint32_t *)&cfg;
    uint32_t words_count = sizeof(CarConfig_t) / 4;

    for (uint32_t i = 0; i < words_count; i++)
    {
        p_dest[i] = p_flash[i];
    }

    if (cfg.magic != FLASH_CONFIG_MAGIC)
    {
        return 0;
    }

    if (cfg.checksum != Config_ComputeChecksum(&cfg))
    {
        return 0;
    }

    s_pid.kp = cfg.kp;
    s_pid.ki = cfg.ki;
    s_pid.kd = cfg.kd;
    s_pid.base_speed = cfg.base_speed;
    s_pid.max_speed = cfg.max_speed;
    s_pid.min_speed = cfg.min_speed;
    if (s_pid.min_speed > 0)
    {
        s_pid.min_speed = 0;
    }
    if (s_pid.min_speed < -80)
    {
        s_pid.min_speed = -80;
    }
    s_line_mode = cfg.line_mode;
    s_corner_advance_ms = cfg.corner_advance_ms;
    if (s_corner_advance_ms == 0 || s_corner_advance_ms > 200)
    {
        s_corner_advance_ms = 70; /* Mac dinh 70ms neu chua tung cai dat */
    }

    QTR8A_SetMinValues(cfg.sensor_min);
    QTR8A_SetMaxValues(cfg.sensor_max);

    return 1;
}

void Car_Init(void)
{
    /* 1. Khoi tao delay SysTick */
    Delay_Init();

    /* 2. Khoi tao Driver dong co TB6612 */
    Motor_Init();
    Motor_Stop();

    /* 3. Khoi tao nut bam PB3, PB4, PB5, PB6 (kem Remap tat JTAG) */
    Button_Init();

    /* 4. Khoi tao mang 8 cap LED + Quang tro qua ADC1 Scan Mode + DMA1 Circular (PA0..PA7) */
    QTR8A_Init();

    /* 5. Khoi tao OLED 128x64 tren PB10, PB11 */
    OLED_Init();

    /* 6. Khoi tao bo dieu khien PID voi thong so chuan mac dinh (min_speed = -55 ho tro pivot turn) */
    PID_Init(&s_pid, 0.040f, 0.000f, 0.350f, 45, 80, -55);

    /* 7. Tu dong doc va khoi phuc thong so tu Flash Page 63 (neu da tung luu) */
    uint8_t flash_ok = Config_LoadFromFlash();

    /* Hien thi man hinh chao mung ban dau */
    OLED_Clear();
    OLED_ShowString(14, 8, "LINE FOLLOWER", 16, OLED_COLOR_WHITE);
    OLED_ShowString(26, 30, "STM32F103", 16, OLED_COLOR_WHITE);
    if (flash_ok)
    {
        OLED_ShowString(14, 52, "[FLASH RESTORED]", 8, OLED_COLOR_WHITE);
    }
    else
    {
        OLED_ShowString(18, 52, "[DEFAULT READY]", 8, OLED_COLOR_WHITE);
    }
    OLED_Update();
    delay_ms(1000);
}

/*
 * MAN HINH CHINH (STOPPED):
 * Chi hien thong tin xe: Tieu de, Vi tri cam bien, Toc do, Che do Line va do thi 8 quang tro
 * Don sach toan bo cai dat vao Menu cho gon gang!
 */
static void UI_DrawStopped(void)
{
    char buf[24];
    OLED_Clear();

    /* Dong 0: Tieu de xe to, dep, can giua */
    OLED_ShowString(14, 0, "LINE FOLLOWER", 16, OLED_COLOR_WHITE);

    /* Dong 1: Vi tri tam line va Do sai lech thuc te */
    snprintf(buf, sizeof(buf), "Pos:%4d   Err:%+5d", s_line_pos, s_line_pos - 3500);
    OLED_ShowString(4, 18, buf, 8, OLED_COLOR_WHITE);

    /* Dong 2: Thong tin toc do va Mau line hien tai */
    snprintf(buf, sizeof(buf), "Spd: %2d%%   Line: %s", s_pid.base_speed, 
             (s_line_mode == QTR8A_LINE_BLACK_WHITE) ? "Den" : "Trang");
    OLED_ShowString(4, 28, buf, 8, OLED_COLOR_WHITE);

    /* Dong 3: Huong dan 2 thao tac chinh: B1 chay xe, B2 vao Menu cai dat */
    OLED_ShowString(10, 38, "B1:CHAY     B2:MENU", 8, OLED_COLOR_WHITE);

    /* Do thi 8 cam bien quang tro phia duoi cung man hinh */
    OLED_DrawSensorBars(s_sensors, s_line_pos);

    OLED_Update();
}

/*
 * MAN HINH KHI XE CHAY (RUNNING):
 * Hien thi truc quan toan dien: Toc do, huong chuyen dong tung banh,
 * thuoc do do lech line va 2 thanh bar cong suat PWM dong co.
 */
static void UI_DrawRunning(void)
{
    char buf[24];
    OLED_Clear();

    /* Dong 0: Thanh banner noi bat RUNNING nhin ro tu xa */
    OLED_FillRect(0, 0, 128, 14, OLED_COLOR_WHITE);
    if (s_active_corner == CORNER_LEFT_90)
    {
        if (s_corner_maneuver == CORNER_MANEUVER_ADVANCE)
        {
            OLED_ShowString(10, 0, "<= 90L TIEN <=", 16, OLED_COLOR_BLACK);
        }
        else
        {
            OLED_ShowString(10, 0, "<= 90L TURN <=", 16, OLED_COLOR_BLACK);
        }
    }
    else if (s_active_corner == CORNER_RIGHT_90)
    {
        if (s_corner_maneuver == CORNER_MANEUVER_ADVANCE)
        {
            OLED_ShowString(10, 0, "=> 90R TIEN =>", 16, OLED_COLOR_BLACK);
        }
        else
        {
            OLED_ShowString(10, 0, "=> 90R TURN =>", 16, OLED_COLOR_BLACK);
        }
    }
    else
    {
        OLED_ShowString(20, 0, "= RUNNING =", 16, OLED_COLOR_BLACK);
    }

    /* Dong 1: Toc do & Huong chuyen dong tung banh xe (L và R) */
    const char *st_l;
    if (s_corner_maneuver == CORNER_MANEUVER_BRAKE) st_l = "PHAN";
    else if (s_corner_maneuver == CORNER_MANEUVER_ADVANCE) st_l = "TRON";
    else if (s_speed_left > 0) st_l = "TIEN";
    else if (s_speed_left < 0) st_l = "LUI ";
    else st_l = "DUNG";

    const char *st_r;
    if (s_corner_maneuver == CORNER_MANEUVER_BRAKE) st_r = "PHAN";
    else if (s_corner_maneuver == CORNER_MANEUVER_ADVANCE) st_r = "TRON";
    else if (s_speed_right > 0) st_r = "TIEN";
    else if (s_speed_right < 0) st_r = "LUI ";
    else st_r = "DUNG";

    snprintf(buf, sizeof(buf), "L:%+3d %-4s R:%+3d %-4s", s_speed_left, st_l, s_speed_right, st_r);
    OLED_ShowString(1, 16, buf, 8, OLED_COLOR_WHITE);

    /* Dong 2: Thuoc do do lech tam line truc quan (Visual Gauge) */
    OLED_ShowChar(0, 26, 'L', 8, OLED_COLOR_WHITE);
    OLED_ShowChar(122, 26, 'R', 8, OLED_COLOR_WHITE);
    OLED_DrawLine(10, 29, 118, 29, OLED_COLOR_WHITE);
    OLED_DrawLine(64, 26, 64, 32, OLED_COLOR_WHITE); /* Vach tim trung tam */

    int16_t p = s_line_pos;
    if (p < 0) p = 0;
    if (p > 7000) p = 7000;
    uint8_t cur_x = 10 + (uint8_t)((uint32_t)p * 108U / 7000U);
    OLED_FillRect(cur_x - 2, 27, 5, 5, OLED_COLOR_WHITE); /* Con tro lech tam */

    /* Dong 3: Sai so vach line va mau line */
    snprintf(buf, sizeof(buf), "Err: %+5d   Line:%s", s_line_pos - 3500, (s_line_mode == QTR8A_LINE_BLACK_WHITE) ? "Den" : "Trg");
    OLED_ShowString(4, 38, buf, 8, OLED_COLOR_WHITE);

    /* Dong 4: Huong dan nut dung */
    OLED_ShowString(10, 48, "(B1: DUNG KHAN CAP)", 8, OLED_COLOR_WHITE);

    /* Dong 5: 2 thanh do do thi cong suat PWM 2 banh (Power Bars) o day man hinh */
    int16_t abs_l = (s_speed_left < 0) ? -s_speed_left : s_speed_left;
    if (abs_l > 100) abs_l = 100;
    uint8_t bar_l = (uint8_t)((uint32_t)abs_l * 54U / 100U);
    OLED_DrawRect(4, 58, 56, 5, OLED_COLOR_WHITE);
    if (bar_l > 0) OLED_FillRect(5, 59, bar_l, 3, OLED_COLOR_WHITE);

    int16_t abs_r = (s_speed_right < 0) ? -s_speed_right : s_speed_right;
    if (abs_r > 100) abs_r = 100;
    uint8_t bar_r = (uint8_t)((uint32_t)abs_r * 54U / 100U);
    OLED_DrawRect(68, 58, 56, 5, OLED_COLOR_WHITE);
    if (bar_r > 0) OLED_FillRect(69, 59, bar_r, 3, OLED_COLOR_WHITE);

    OLED_Update();
}

/*
 * MENU CAI DAT: Don tat ca thong so vao Menu cho gon:
 * 0. Toc do (Speed)
 * 1. Do nhay (Kp)
 * 2. Giam rung (Kd)
 * 3. Mau Line (Den / Trang)
 * 4. Hieu chuan (Calib quet tay)
 * 5. Luu Flash (Save)
 * 6. Thoat ve man hinh chinh
 */
static void UI_DrawMenu(void)
{
    OLED_Clear();

    /* Tieu de */
    OLED_ShowString(14, 0, "=== MENU CAI DAT ===", 8, OLED_COLOR_WHITE);

    /* Tinh toan 4 muc hien thi quanh con tro */
    uint8_t start_idx = 0;
    if (s_menu_item >= 4)
    {
        start_idx = s_menu_item - 3;
    }
    if (start_idx > MENU_TOTAL_ITEMS - 4)
    {
        start_idx = MENU_TOTAL_ITEMS - 4;
    }

    char labels[MENU_TOTAL_ITEMS][22];
    snprintf(labels[0], sizeof(labels[0]), "1.Toc do:  %2d%%", s_pid.base_speed);
    snprintf(labels[1], sizeof(labels[1]), "2.Kp:      %.3f", s_pid.kp);
    snprintf(labels[2], sizeof(labels[2]), "3.Kd:      %.3f", s_pid.kd);
    snprintf(labels[3], sizeof(labels[3]), "4.Phanh:   %2d%%", (int16_t)(-s_pid.min_speed));
    snprintf(labels[4], sizeof(labels[4]), "5.Truon:  %3dms", s_corner_advance_ms);
    snprintf(labels[5], sizeof(labels[5]), "6.Line:    %s", (s_line_mode == QTR8A_LINE_BLACK_WHITE) ? "Den" : "Trang");
    snprintf(labels[6], sizeof(labels[6]), "7.Calib:   [Start]");
    snprintf(labels[7], sizeof(labels[7]), "8.Luu:     [Flash]");
    snprintf(labels[8], sizeof(labels[8]), "9.Thoat ve Trang chu");

    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t idx = start_idx + i;
        uint8_t y = 12 + i * 10;
        if (idx == s_menu_item)
        {
            OLED_ShowChar(0, y, '>', 8, OLED_COLOR_WHITE);
        }
        else
        {
            OLED_ShowChar(0, y, ' ', 8, OLED_COLOR_WHITE);
        }
        OLED_ShowString(8, y, labels[idx], 8, OLED_COLOR_WHITE);
    }

    /* Huong dan nut bam o day menu */
    OLED_ShowString(6, 54, "B4:v  B3:+  B2:-  B1:OK", 8, OLED_COLOR_WHITE);

    OLED_Update();
}

static void Do_Calibration(void)
{
    /* 1. Tat dong co hoan toan de nguoi dung cam tay quet xe tren line */
    Motor_Stop();

    /* 2. Xoa gia tri min/max cu de bat dau lay mau moi */
    QTR8A_ResetCalibration();

    uint32_t last_disp = 0;

    while (1)
    {
        /* Cap nhat trang thai nut bam */
        Button_Update();

        /* Doc va cap nhat min/max tu bo dem DMA */
        QTR8A_Calibrate();
        delay_ms(5);

        /* Nguoi dung quet tay xong bam PB3 (B1) hoac PB4 (B2) de xac nhan xong */
        if (Button_WasPressed(BTN_RUN) || Button_WasPressed(BTN_MENU))
        {
            break;
        }

        /* Cap nhat giao dien OLED ~15Hz */
        uint32_t now = millis();
        if (now - last_disp >= 65)
        {
            last_disp = now;
            OLED_Clear();

            /* Tieu de va huong dan */
            OLED_ShowString(16, 0, "= MANUAL CALIB =", 8, OLED_COLOR_WHITE);
            OLED_ShowString(4, 14, "Quet tay qua line...", 8, OLED_COLOR_WHITE);
            OLED_ShowString(8, 26, "Xong bam B1/B2 (OK)", 8, OLED_COLOR_WHITE);

            /* Hien thi muc do chenh lech (Max - Min) cua 8 cam bien duoi dang 8 cot */
            const uint16_t *mins = QTR8A_GetMinValues();
            const uint16_t *maxs = QTR8A_GetMaxValues();
            uint16_t contrast[QTR8A_NUM_SENSORS];

            for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
            {
                if (maxs[i] > mins[i])
                {
                    uint32_t diff = maxs[i] - mins[i];
                    contrast[i] = (uint16_t)((diff * 1000U) / 4095U);
                }
                else
                {
                    contrast[i] = 0;
                }
            }

            /* Ve 8 cot do tuong phan */
            OLED_DrawSensorBars(contrast, 3500);

            OLED_Update();
        }
    }

    Motor_Stop();

    /* 3. Tu dong luu thong so va Min/Max vua Calib vao Flash Page 63 */
    uint8_t save_ok = Config_SaveToFlash();

    /* Thong bao hieu chuan thanh cong */
    OLED_Clear();
    OLED_ShowString(24, 14, "HIEU CHUAN", 16, OLED_COLOR_WHITE);
    OLED_ShowString(20, 34, "THANH CONG!", 16, OLED_COLOR_WHITE);
    if (save_ok)
    {
        OLED_ShowString(16, 52, "DA LUU VAO FLASH", 8, OLED_COLOR_WHITE);
    }
    OLED_Update();
    delay_ms(900);
}

void Car_Loop(void)
{
    static uint32_t last_ui_tick = 0;
    static uint32_t last_pid_tick = 0;
    uint32_t now = millis();

    /* 1. Cap nhat nut bam lien tuc */
    Button_Update();

    /* 2. May trang thai dieu khien (State Machine) */
    switch (s_state)
    {
        case CAR_STATE_STOPPED:
        {
            /* Doc cam bien tu RAM DMA de hien thi truc tiep len OLED khi dung xe */
            s_line_pos = QTR8A_ReadLine(s_sensors, s_line_mode);

            /* Xu ly nut bam o man hinh chinh: Chi gom 2 chuc nang chinh */
            if (Button_WasPressed(BTN_RUN))
            {
                /* Nut 1 (PB3): Bat dau chay xe */
                PID_Reset(&s_pid);
                s_state = CAR_STATE_RUNNING;
            }
            else if (Button_WasPressed(BTN_MENU))
            {
                /* Nut 2 (PB4): Mo Menu cai dat */
                s_state = CAR_STATE_MENU;
            }

            /* Cap nhat OLED ~20Hz */
            if (now - last_ui_tick >= 50)
            {
                last_ui_tick = now;
                UI_DrawStopped();
            }
            break;
        }

        case CAR_STATE_RUNNING:
        {
            /* Nhan bat ky nut nao de DUNG xe khan cap ngay lap tuc */
            if (Button_WasPressed(BTN_RUN) || Button_WasPressed(BTN_MENU) || 
                Button_WasPressed(BTN_UP) || Button_WasPressed(BTN_DOWN))
            {
                Motor_Stop();
                s_corner_maneuver = CORNER_MANEUVER_IDLE;
                s_active_corner = CORNER_NONE;
                s_state = CAR_STATE_STOPPED;
                break;
            }

            /* Vong lap tinh toan PID moi 5ms (200Hz) */
            if (now - last_pid_tick >= 5)
            {
                last_pid_tick = now;

                /* 1. Doc vi tri line tu mang quang tro qua DMA (0..7000, 3500 la tam, CPU wait = 0us) */
                s_line_pos = QTR8A_ReadLine(s_sensors, s_line_mode);

                /* Kiem tra xem co it nhat 1 mat nhin thay line khong */
                uint8_t on_line = QTR8A_IsOnLine(s_sensors);

                /* 2. Xu ly may trang thai goc cua 90 do (Sharp 90-degree Corner State Machine) */
                if (s_corner_maneuver == CORNER_MANEUVER_IDLE)
                {
                    /* Kiem tra nhan dien goc vuong 90 do tu cam bien */
                    CornerType_t detected = QTR8A_DetectCorner(s_sensors);
                    if (detected == CORNER_NONE && !on_line)
                    {
                        if (s_line_pos <= 2000) detected = CORNER_LEFT_90;
                        else if (s_line_pos >= 5000) detected = CORNER_RIGHT_90;
                    }

                    if (detected != CORNER_NONE)
                    {
                        s_active_corner = detected;
                        s_corner_step_tick = now;

                        if (s_corner_advance_ms > 0)
                        {
                            /* Pha 1: Truon toi dinh goc (Forward Advance) de truc banh xe tien toi dung dinh goc */
                            s_corner_maneuver = CORNER_MANEUVER_ADVANCE;
                            s_speed_left = s_pid.base_speed;
                            s_speed_right = s_pid.base_speed;
                            Motor_SetSpeed(s_speed_left, s_speed_right);
                        }
                        else
                        {
                            /* Bo qua pha truon neu cai dat = 0ms, phanh ngay */
                            s_corner_maneuver = CORNER_MANEUVER_BRAKE;
                            Motor_Brake();
                        }
                    }
                }

                if (s_corner_maneuver == CORNER_MANEUVER_ADVANCE)
                {
                    /* Duy tri toc do tien deu de dua truc banh xe toi dung dinh goc cua */
                    s_speed_left = s_pid.base_speed;
                    s_speed_right = s_pid.base_speed;
                    Motor_SetSpeed(s_speed_left, s_speed_right);

                    if (now - s_corner_step_tick >= s_corner_advance_ms)
                    {
                        s_corner_maneuver = CORNER_MANEUVER_BRAKE;
                        s_corner_step_tick = now;
                        Motor_Brake(); /* Phanh gap de triet tieu quan tinh tien ngay tai dinh goc */
                    }
                }
                else if (s_corner_maneuver == CORNER_MANEUVER_BRAKE)
                {
                    /* Thoi gian phanh chu dong (25ms - 35ms) de triet tieu quan tinh tien cua xe nang JGB37-520 */
                    uint32_t brake_time = 25 + (uint32_t)(-s_pid.min_speed) * 15U / 100U;
                    Motor_Brake();
                    if (now - s_corner_step_tick >= brake_time)
                    {
                        s_corner_maneuver = CORNER_MANEUVER_PIVOT;
                        s_corner_step_tick = now;
                    }
                }
                else if (s_corner_maneuver == CORNER_MANEUVER_PIVOT)
                {
                    /* Tinh toc do quay tai cho (Spin Turn Speed) theo muc phanh da cai dat */
                    int16_t p_spd = -s_pid.min_speed;
                    if (p_spd < 55) p_spd = 55;
                    if (p_spd > 75) p_spd = 75;

                    if (s_active_corner == CORNER_LEFT_90)
                    {
                        /* Quay sang phia trai (huong mat 0..1):
                         * Quy uoc dong co thuc te tren xe (da kiem chung qua pid.c):
                         * Motor A (speed_left) TIEN (+), Motor B (speed_right) LUI (-) */
                        s_speed_left = p_spd;
                        s_speed_right = -p_spd;
                    }
                    else
                    {
                        /* Quay sang phia phai (huong mat 6..7):
                         * Motor A (speed_left) LUI (-), Motor B (speed_right) TIEN (+) */
                        s_speed_left = -p_spd;
                        s_speed_right = p_spd;
                    }
                    Motor_SetSpeed(s_speed_left, s_speed_right);

                    /* Dieu kien thoat goc cua:
                     * 1. Phai quay it nhat 120ms de thoat khoi vung goc cu va tao da xoay cho xe 1kg JGB37
                     * 2. Va cac mat trung tam (mat 2, 3, 4 hoac 5) da bat lai duoc vach line moi
                     * 3. Hoac timeout bao ve 750ms tranh quay vo tan neu bi nhac khoi san
                     */
                    uint32_t elapsed = now - s_corner_step_tick;
                    if (elapsed >= 120)
                    {
                        if (on_line && (s_sensors[2] > 280 || s_sensors[3] > 280 || s_sensors[4] > 280 || s_sensors[5] > 280))
                        {
                            /* Da bat duoc line moi o trung tam! */
                            s_corner_maneuver = CORNER_MANEUVER_IDLE;
                            s_active_corner = CORNER_NONE;
                            PID_Reset(&s_pid); /* Xoa tich phan I de khong bi giat line */
                        }
                        else if (elapsed >= 750)
                        {
                            /* Timeout an toan */
                            s_corner_maneuver = CORNER_MANEUVER_IDLE;
                            s_active_corner = CORNER_NONE;
                            PID_Reset(&s_pid);
                        }
                    }
                }
                else
                {
                    /* 3. Tinh toan tin hieu sai so va dau ra PID (kem giam toc vao cua dong) */
                    s_pid_out = PID_Compute(&s_pid, s_line_pos, 3500);

                    /* 4. Tinh toan toc do 2 dong co tu PID output */
                    PID_CalculateMotorSpeeds(&s_pid, s_pid_out, &s_speed_left, &s_speed_right);

                    /* 5. Xuat PWM toi dong co */
                    Motor_SetSpeed(s_speed_left, s_speed_right);
                }
            }

            /* Cap nhat OLED ~8Hz (moi 120ms) khi dang chay de khong anh huong toi chu ky PID 5ms */
            if (now - last_ui_tick >= 120)
            {
                last_ui_tick = now;
                UI_DrawRunning();
            }
            break;
        }

        case CAR_STATE_CALIBRATING:
        {
            Do_Calibration();
            s_state = CAR_STATE_STOPPED;
            break;
        }

        case CAR_STATE_MENU:
        {
            /* BTN_DOWN (PB6): Di chuyen con tro xuong muc tiep theo (0 -> 1 -> ... -> 6 -> 0) */
            if (Button_WasPressed(BTN_DOWN))
            {
                s_menu_item = (s_menu_item + 1) % MENU_TOTAL_ITEMS;
            }

            /* BTN_UP (PB5): Tang gia tri (+) hoac thuc thi lenh */
            if (Button_WasPressed(BTN_UP))
            {
                switch (s_menu_item)
                {
                    case 0: /* Toc do (Base Speed) */
                        s_pid.base_speed += 5;
                        if (s_pid.base_speed > 100) s_pid.base_speed = 100;
                        s_pid.max_speed = s_pid.base_speed + 35;
                        if (s_pid.max_speed > 100) s_pid.max_speed = 100;
                        break;
                    case 1: /* Kp */
                        s_pid.kp += 0.005f;
                        break;
                    case 2: /* Kd */
                        s_pid.kd += 0.020f;
                        break;
                    case 3: /* Phanh (Tang luc phanh: 0% -> 5% -> ... -> 80%) */
                    {
                        int16_t brk = -s_pid.min_speed;
                        brk += 5;
                        if (brk > 80) brk = 80;
                        s_pid.min_speed = -brk;
                        break;
                    }
                    case 4: /* Truon (Tang thoi gian truon toi dinh goc: 0ms -> 10ms -> ... -> 200ms) */
                        if (s_corner_advance_ms <= 190) s_corner_advance_ms += 10;
                        else s_corner_advance_ms = 200;
                        break;
                    case 5: /* Mau Line */
                        s_line_mode = !s_line_mode;
                        break;
                    case 6: /* Calib */
                        s_state = CAR_STATE_CALIBRATING;
                        break;
                    case 7: /* Save Flash */
                    {
                        uint8_t ok = Config_SaveToFlash();
                        OLED_Clear();
                        if (ok)
                        {
                            OLED_ShowString(20, 24, "DA LUU FLASH!", 16, OLED_COLOR_WHITE);
                        }
                        else
                        {
                            OLED_ShowString(16, 24, "LOI GHI FLASH", 16, OLED_COLOR_WHITE);
                        }
                        OLED_Update();
                        delay_ms(700);
                        break;
                    }
                    case 8: /* Thoat ve Trang chu */
                        Config_SaveToFlash();
                        s_state = CAR_STATE_STOPPED;
                        break;
                    default: break;
                }
            }

            /* BTN_MENU (PB4): Giam gia tri (-) hoac thuc thi lenh */
            if (Button_WasPressed(BTN_MENU))
            {
                switch (s_menu_item)
                {
                    case 0: /* Toc do (Base Speed) */
                        s_pid.base_speed -= 5;
                        if (s_pid.base_speed < 15) s_pid.base_speed = 15;
                        s_pid.max_speed = s_pid.base_speed + 35;
                        if (s_pid.max_speed > 100) s_pid.max_speed = 100;
                        break;
                    case 1: /* Kp */
                        s_pid.kp -= 0.005f;
                        if (s_pid.kp < 0.0f) s_pid.kp = 0.0f;
                        break;
                    case 2: /* Kd */
                        s_pid.kd -= 0.020f;
                        if (s_pid.kd < 0.0f) s_pid.kd = 0.0f;
                        break;
                    case 3: /* Phanh (Giam luc phanh: 80% -> 75% -> ... -> 0%) */
                    {
                        int16_t brk = -s_pid.min_speed;
                        brk -= 5;
                        if (brk < 0) brk = 0;
                        s_pid.min_speed = -brk;
                        break;
                    }
                    case 4: /* Truon (Giam thoi gian truon toi dinh goc: 200ms -> 190ms -> ... -> 0ms) */
                        if (s_corner_advance_ms >= 10) s_corner_advance_ms -= 10;
                        else s_corner_advance_ms = 0;
                        break;
                    case 5: /* Mau Line */
                        s_line_mode = !s_line_mode;
                        break;
                    case 6: /* Calib */
                        s_state = CAR_STATE_CALIBRATING;
                        break;
                    case 7: /* Save Flash */
                    {
                        uint8_t ok = Config_SaveToFlash();
                        OLED_Clear();
                        if (ok)
                        {
                            OLED_ShowString(20, 24, "DA LUU FLASH!", 16, OLED_COLOR_WHITE);
                        }
                        else
                        {
                            OLED_ShowString(16, 24, "LOI GHI FLASH", 16, OLED_COLOR_WHITE);
                        }
                        OLED_Update();
                        delay_ms(700);
                        break;
                    }
                    case 8: /* Thoat ve Trang chu */
                        Config_SaveToFlash();
                        s_state = CAR_STATE_STOPPED;
                        break;
                    default: break;
                }
            }

            /* BTN_RUN (PB3): Thoat Menu, tu dong luu thong so va ve lai STOPPED */
            if (Button_WasPressed(BTN_RUN))
            {
                Config_SaveToFlash();
                s_state = CAR_STATE_STOPPED;
            }

            if (now - last_ui_tick >= 50)
            {
                last_ui_tick = now;
                UI_DrawMenu();
            }
            break;
        }
    }
}
