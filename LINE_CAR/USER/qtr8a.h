#ifndef __QTR8A_H
#define __QTR8A_H

#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include <stdint.h>

/*
 * Driver mảng 8 cặp LED + Quang trở (LDR / Photoresistor) hoặc QTR-8A
 * Kết nối: PA0 -> PA7 tương ứng ADC1 Kênh 0 -> 7
 * Cơ chế: ADC1 Scan Mode + DMA1 Channel 1 Circular Buffer (Đọc liên tục, 0us CPU wait)
 * Tích hợp bộ lọc trung bình trượt 4 khung mẫu (Oversampling) khử nhiễu quang trở
 */

#define QTR8A_NUM_SENSORS        8
#define QTR8A_OVERSAMPLE_COUNT   4  /* 4 mẫu liên tiếp cho mỗi mắt cảm biến */

#define QTR8A_LINE_BLACK_WHITE   0  /* Line đen trên nền trắng */
#define QTR8A_LINE_WHITE_BLACK   1  /* Line trắng trên nền đen */

/* Khởi tạo ADC1 + DMA1 Channel 1 cho mảng 8 quang trở */
void QTR8A_Init(void);

/* Đọc giá trị thô ADC (0..4095) đã qua lọc DMA */
void QTR8A_ReadRaw(uint16_t *raw_values);

/* Quản lý quá trình hiệu chuẩn Min/Max ngoài sân */
void QTR8A_ResetCalibration(void);
void QTR8A_Calibrate(void);
void QTR8A_ReadCalibrated(uint16_t *calibrated_values);

/* Tính toán vị trí tâm line (0..7000, tâm chuẩn = 3500) */
int16_t QTR8A_ReadLine(uint16_t *sensor_values, uint8_t line_mode);

/* Đọc và ghi trực tiếp giá trị Min/Max (dùng để lưu/khôi phục từ Flash) */
const uint16_t* QTR8A_GetMinValues(void);
const uint16_t* QTR8A_GetMaxValues(void);
void QTR8A_SetMinValues(const uint16_t *min_values);
void QTR8A_SetMaxValues(const uint16_t *max_values);

/* Nhận diện vạch ngang xuất phát / vạch đích (Crossline) */
uint8_t QTR8A_IsCrossLine(const uint16_t *sensor_values);

/* Kiem tra xem co it nhat 1 mat nhin thay vach line khong */
uint8_t QTR8A_IsOnLine(const uint16_t *sensor_values);

/* Phan loai goc cua 90 do */
typedef enum {
    CORNER_NONE = 0,
    CORNER_LEFT_90,
    CORNER_RIGHT_90
} CornerType_t;

/* Nhan dien goc vuong 90 do tu mang cam bien quang tro */
CornerType_t QTR8A_DetectCorner(const uint16_t *sensor_values);

#endif /* __QTR8A_H */
