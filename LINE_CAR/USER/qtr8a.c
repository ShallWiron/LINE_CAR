#include "qtr8a.h"

#define TOTAL_DMA_BUFFER_SIZE  (QTR8A_NUM_SENSORS * QTR8A_OVERSAMPLE_COUNT)

/*
 * Bo dem DMA nhan du lieu tu dong tu ADC1.
 * Voi 4 mau (Oversample Count = 4), mang 32 phan tu luu 4 luot quet lien tiep cua 8 kenh:
 * Luot 0: buffer[0..7]
 * Luot 1: buffer[8..15]
 * Luot 2: buffer[16..23]
 * Luot 3: buffer[24..31]
 */
static volatile uint16_t s_adc_dma_buffer[TOTAL_DMA_BUFFER_SIZE];

static uint16_t s_sensor_min[QTR8A_NUM_SENSORS];
static uint16_t s_sensor_max[QTR8A_NUM_SENSORS];
static int16_t s_last_position = 3500;

void QTR8A_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    /* 1. Cap clock cho GPIOA, ADC1 va DMA1 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* 2. Chia clock ADC: 72MHz / 6 = 12MHz (muc toi da an toan cua STM32F1 la 14MHz) */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* 3. Cau hinh PA0..PA7 lam Analog Input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
                                  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 4. Cau hinh DMA1 Channel 1 (ket noi co dinh voi ADC1 tren STM32F103) */
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)s_adc_dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = TOTAL_DMA_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* 5. Cau hinh ADC1: Che do Scan Mode + Continuous Conversion */
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;          /* Quet lien tuc qua 8 kenh */
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;    /* Tu dong quet vong lap khong can kich lai */
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = QTR8A_NUM_SENSORS;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* 6. Gan 8 kenh vao cac Rank tu 1 den 8
     * QUAN TRONG CHO QUANG TRO (LDR):
     * Cau phan ap quang tro co tro khang lon hon photodiode/phototransistor,
     * vi vay chon ADC_SampleTime_71Cycles5 (hoac 239.5) giup tu lay mau cua ADC
     * nạp du thoi gian, triet tieu hoan toan hien tuong sai so do tro khang cao */
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        ADC_RegularChannelConfig(ADC1, i, i + 1, ADC_SampleTime_71Cycles5);
    }

    /* 7. Kich hoat DMA cho ADC1 */
    ADC_DMACmd(ADC1, ENABLE);

    /* 8. Bat ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    /* 9. Hieu chuan ADC1 */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    /* 10. Kich hoat phan mem bat dau chu trinh chuyen doi lien tuc.
     * Sau lenh nay, phan cung ADC va DMA tu dong lam viec ngam 100%,
     * CPU khong mat bat cu chu ky doi nao */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* 11. Khoi tao gia tri min/max ban dau */
    QTR8A_ResetCalibration();
}

void QTR8A_ResetCalibration(void)
{
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        s_sensor_min[i] = 4095;
        s_sensor_max[i] = 0;
    }
}

void QTR8A_ReadRaw(uint16_t *raw_values)
{
    /* Tinh trung binh 4 mau lien tiep da duoc DMA thu thap tu dong.
     * Thao tac chi gom phep cong va dich bit (>> 2), thoi gian xu ly < 1 microsecond!
     * Giup triet tieu triet de nhieu tan so cao tu quang tro va rung co khi */
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        uint32_t sum = (uint32_t)s_adc_dma_buffer[i] +
                       (uint32_t)s_adc_dma_buffer[i + 8] +
                       (uint32_t)s_adc_dma_buffer[i + 16] +
                       (uint32_t)s_adc_dma_buffer[i + 24];

        raw_values[i] = (uint16_t)(sum >> 2);
    }
}

void QTR8A_Calibrate(void)
{
    uint16_t raw[QTR8A_NUM_SENSORS];
    QTR8A_ReadRaw(raw);

    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        if (raw[i] < s_sensor_min[i])
        {
            s_sensor_min[i] = raw[i];
        }
        if (raw[i] > s_sensor_max[i])
        {
            s_sensor_max[i] = raw[i];
        }
    }
}

void QTR8A_ReadCalibrated(uint16_t *calibrated_values)
{
    uint16_t raw[QTR8A_NUM_SENSORS];
    QTR8A_ReadRaw(raw);

    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        uint16_t r = raw[i];
        uint16_t min_v = s_sensor_min[i];
        uint16_t max_v = s_sensor_max[i];

        if (max_v <= min_v)
        {
            /* Chua qua hieu chuan, su dung ty le tuyet doi */
            calibrated_values[i] = (r * 1000U) / 4095U;
        }
        else
        {
            int32_t val;
            if (r <= min_v)
            {
                val = 0;
            }
            else if (r >= max_v)
            {
                val = 1000;
            }
            else
            {
                val = ((int32_t)(r - min_v) * 1000) / (int32_t)(max_v - min_v);
            }
            calibrated_values[i] = (uint16_t)val;
        }
    }
}

int16_t QTR8A_ReadLine(uint16_t *sensor_values, uint8_t line_mode)
{
    uint16_t cal[QTR8A_NUM_SENSORS];
    QTR8A_ReadCalibrated(cal);

    uint32_t avg = 0;
    uint32_t sum = 0;
    uint8_t on_line = 0;

    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        uint16_t val = cal[i];

        /* Dao gia tri neu di tren line trang nen den */
        if (line_mode == QTR8A_LINE_WHITE_BLACK)
        {
            val = 1000 - val;
        }

        /* Luu gia tri tra ve cho nguoi dung neu truyen buffer */
        if (sensor_values != 0)
        {
            sensor_values[i] = val;
        }

        /* Nguong nhan dien cam bien dang tren vach line (> 20% thang do) */
        if (val > 200)
        {
            on_line = 1;
        }

        /* Bo loc nhieu nen: chi tinh tong nhung mat co do tuong phan tren 5% */
        if (val > 50)
        {
            avg += (uint32_t)val * (i * 1000U);
            sum += val;
        }
    }

    /* Neu tat ca 8 mat quang tro deu khong thay line (bi mat line) */
    if (!on_line || sum == 0)
    {
        /* Tra ve vi tri cuc tri de xe quay lai tim line */
        if (s_last_position < 3500)
        {
            return 0;      /* Da tung o nua trai -> be lai toi da trai */
        }
        else
        {
            return 7000;   /* Da tung o nua phai -> be lai toi da phai */
        }
    }

    s_last_position = (int16_t)(avg / sum);
    return s_last_position;
}

const uint16_t* QTR8A_GetMinValues(void)
{
    return s_sensor_min;
}

const uint16_t* QTR8A_GetMaxValues(void)
{
    return s_sensor_max;
}

void QTR8A_SetMinValues(const uint16_t *min_values)
{
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        s_sensor_min[i] = min_values[i];
    }
}

void QTR8A_SetMaxValues(const uint16_t *max_values)
{
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        s_sensor_max[i] = max_values[i];
    }
}

uint8_t QTR8A_IsCrossLine(const uint16_t *sensor_values)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        /* Vach den tren nen trang: do tuong phan tren 65% */
        if (sensor_values[i] > 650)
        {
            count++;
        }
    }
    /* Co tu 6 mat tro len dong thoi bat duoc vach den -> Xac nhan vach ngang */
    return (count >= 6) ? 1 : 0;
}

uint8_t QTR8A_IsOnLine(const uint16_t *sensor_values)
{
    for (uint8_t i = 0; i < QTR8A_NUM_SENSORS; i++)
    {
        if (sensor_values[i] > 200)
        {
            return 1;
        }
    }
    return 0;
}

CornerType_t QTR8A_DetectCorner(const uint16_t *sensor_values)
{
    /* 1. Loai tru truong hop vach ngang (Crossline / Vach xuat phat / Nga tu) */
    if (QTR8A_IsCrossLine(sensor_values))
    {
        return CORNER_NONE;
    }

    /* Tinh tong do dam nua trai (0..3) va nua phai (4..7) */
    uint32_t sum_left = (uint32_t)sensor_values[0] + sensor_values[1] + 
                        (uint32_t)sensor_values[2] + sensor_values[3];
    uint32_t sum_right = (uint32_t)sensor_values[4] + sensor_values[5] + 
                         (uint32_t)sensor_values[6] + sensor_values[7];

    /* 2. Nhan dien goc vuong ben Trai (Phia mat 0, 1):
     * - Mat ngoai cung trai (0 hoac 1) bat duoc vach (> 300)
     * - Mat ngoai cung phai (7) khong phai vach dam (< 450)
     * - Do bat doi xung trai/phai vuot troi (sum_left > 800 va gap doi sum_right)
     */
    if ((sensor_values[0] > 300 || sensor_values[1] > 360) &&
        sensor_values[7] < 450 &&
        sum_left > 800 && sum_left > (sum_right * 2U))
    {
        return CORNER_LEFT_90;
    }

    /* 3. Nhan dien goc vuong ben Phai (Phia mat 6, 7):
     * - Mat ngoai cung phai (7 hoac 6) bat duoc vach (> 300)
     * - Mat ngoai cung trai (0) khong phai vach dam (< 450)
     * - Do bat doi xung phai/trai vuot troi (sum_right > 800 va gap doi sum_left)
     */
    if ((sensor_values[7] > 300 || sensor_values[6] > 360) &&
        sensor_values[0] < 450 &&
        sum_right > 800 && sum_right > (sum_left * 2U))
    {
        return CORNER_RIGHT_90;
    }

    return CORNER_NONE;
}
