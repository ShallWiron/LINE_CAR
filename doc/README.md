# HỆ THỐNG TÀI LIỆU DỰ ÁN XE DÒ LINE (LINE FOLLOWER CAR)
> **Vi điều khiển:** STM32F103C8T6 (ARM Cortex-M3 @ 72MHz)  
> **Động cơ chấp hành:** 2 Động cơ DC giảm tốc kim loại **JGB37-520** (12V/6V, hộp số thép 37mm)  
> **Bộ điều khiển động cơ:** TB6612FNG (Dual H-Bridge, PWM 1 kHz)  
> **Cảm biến dò line:** Mảng 8 cặp LED + Quang trở (LDR) qua ADC1 + DMA1 Channel 1 Circular  
> **Bộ nhớ cấu hình:** Flash Emulated EEPROM (Trang 63: Tự động lưu/khôi phục PID & Calib)  
> **Màn hình & Giao diện:** OLED 0.96" I2C (SSD1306 128x64) + 4 Nút nhấn  
> **Thuật toán chính:** PID Control + Centroid Position Estimation + FSM + 90° Corner Handler  
> **Môi trường:** Keil MDK-ARM v5 (ARMCLANG v6), STM32F10x Standard Peripheral Library (SPL)

---

## 📂 Danh mục tài liệu

Để không phải đọc lại toàn bộ mã nguồn khi bảo trì hoặc phát triển thêm, tài liệu kỹ thuật được phân chia thành các phần chi tiết sau:

| STT | Tài liệu | Nội dung chính |
| :---: | :--- | :--- |
| **01** | [**01_TONG_QUAN_VA_CAU_TRUC.md**](./01_TONG_QUAN_VA_CAU_TRUC.md) | Tổng quan hệ thống, môi trường build, cấu trúc file, dung lượng bộ nhớ. |
| **02** | [**02_SO_DO_PHAN_CUNG_PINOUT.md**](./02_SO_DO_PHAN_CUNG_PINOUT.md) | Bảng tra cứu chân (Pinout), kết nối ngoại vi, kỹ thuật Remap PB3/PB4. |
| **03** | [**03_KIEN_TRUC_PHAN_MEM_FSM.md**](./03_KIEN_TRUC_PHAN_MEM_FSM.md) | Kiến trúc phần mềm, máy trạng thái (FSM), chu kỳ thời gian thực, API module. |
| **04** | [**04_THUAT_TOAN_PID_VA_CAM_BIEN.md**](./04_THUAT_TOAN_PID_VA_CAM_BIEN.md) | Công thức trọng số đọc cảm biến, toán tử PID, giải thuật xử lý góc cua 90°. |
| **05** | [**05_HUONG_DAN_VAN_HANH_MENU.md**](./05_HUONG_DAN_VAN_HANH_MENU.md) | Hướng dẫn thao tác 4 nút nhấn, quy trình Calib tay ngoài sân, chỉnh Menu OLED. |
| **06** | [**06_DE_XUAT_NANG_CAP_TOI_UU.md**](./06_DE_XUAT_NANG_CAP_TOI_UU.md) | Kế hoạch tối ưu: Lưu Flash EEPROM, ADC DMA Scan, tối ưu OLED, vạch dừng đích. |
| **07** | [**07_PHAN_TICH_DONG_CO_JGB37_VA_CAC_TRUONG_HOP_VAN_DE.md**](./07_PHAN_TICH_DONG_CO_JGB37_VA_CAC_TRUONG_HOP_VAN_DE.md) | **Phân tích chuyên sâu động cơ tải nặng JGB37-520, các trường hợp trên sân, rủi ro TB6612 và giải pháp.** |

---

## ⚡ Tra cứu nhanh (Cheat Sheet)

### 1. Bảng chân kết nối vi điều khiển STM32F103C8T6
```
                      +-------------------+
             (ADC0)   | PA0           PB3 | Nút BTN_RUN  (Remapped JTAG)
             (ADC1)   | PA1           PB4 | Nút BTN_MODE (Remapped JTAG)
             (ADC2)   | PA2           PB5 | Nút BTN_UP
 QTR-8A      (ADC3)   | PA3           PB6 | Nút BTN_DOWN
 8 Mắt Analog (ADC4)   | PA4               |
             (ADC5)   | PA5          PB10 | OLED SCL (Bit-bang)
             (ADC6)   | PA6          PB11 | OLED SDA (Bit-bang)
             (ADC7)   | PA7          PB12 | TB6612 AIN2 (Motor A)
                      |              PB13 | TB6612 AIN1 (Motor A)
TB6612 PWMB (TIM1_CH1)| PA8          PB14 | TB6612 BIN1 (Motor B)
TB6612 PWMA (TIM1_CH2)| PA9          PB15 | TB6612 BIN2 (Motor B)
                      +-------------------+
```

### 2. Ý nghĩa 4 nút bấm
* **PB3 (`Nút 1 - BTN_RUN`):** Chạy / Dừng xe khẩn cấp; Xác nhận lưu Flash & Thoát Menu (OK).
* **PB4 (`Nút 2 - BTN_MENU`):** Mở Menu cài đặt (ở màn hình chính); Giảm giá trị thông số (`-`) trong Menu.
* **PB5 (`Nút 3 - BTN_UP`):** Tăng giá trị thông số (`+`) / Chọn lệnh trong Menu (hỗ trợ Auto-repeat khi giữ).
* **PB6 (`Nút 4 - BTN_DOWN`):** Di chuyển con trỏ xuống (`v`) trong Menu.

### 3. Thông số PID mặc định
* $K_p = 0.040$, $K_i = 0.000$, $K_d = 0.350$
* **Base Speed:** $45\%$ | **Max Speed:** $80\%$ | **Min Speed:** $-30\%$ (hỗ trợ quay lùi bánh trong khi ôm cua).
* **Chu kỳ PID:** $5\text{ ms}$ ($200\text{ Hz}$).
