# 01. TỔNG QUAN HỆ THỐNG VÀ CẤU TRÚC DỰ ÁN

## 1. Giới thiệu dự án
Dự án **LINE_CAR** là hệ thống xe dò line tự hành sử dụng vi điều khiển STM32F103C8T6 kết hợp thanh cảm biến hồng ngoại tương tự 8 mắt (Pololu QTR-8A) và mạch cầu H kép TB6612FNG. Xe được trang bị màn hình OLED 0.96 inch hiển thị thông số và 4 nút nhấn cho phép người dùng hiệu chuẩn cảm biến và tinh chỉnh trực tiếp các tham số PID ngoài sân mà không cần máy tính hoặc nạp lại firmware.

---

## 2. Thông số kỹ thuật phần cứng

| Thành phần | Đặc tính kỹ thuật |
| :--- | :--- |
| **Vi điều khiển** | STM32F103C8T6 (Cortex-M3, 32-bit, xung nhịp tối đa 72 MHz) |
| **Bộ nhớ nội** | 64 KB Flash, 20 KB SRAM |
| **Cảm biến dò đường** | Pololu QTR-8A (8 mắt thu phát hồng ngoại Analog, phân giải 12-bit qua ADC1) |
| **Driver động cơ** | TB6612FNG (Dòng liên tục 1.2A/kênh, đỉnh 3.2A, hiệu suất cao hơn L298N) |
| **Động cơ chấp hành** | 2 Động cơ DC giảm tốc (Bánh Trái: Motor A, Bánh Phải: Motor B) |
| **Màn hình hiển thị** | OLED 0.96 inch SSD1306, độ phân giải 128x64 pixels, chuẩn giao tiếp I2C |
| **Nút bấm điều khiển** | 4 nút nhấn dạng tactile switch kéo chân xuống GND qua trở kéo nội IPU |
| **Nguồn cấp** | Pin LiPo 2S (7.4V - 8.4V) hoặc 3S (11.1V), hạ áp 5V (cấp OLED/Driver) và 3.3V (cấp MCU) |

---

## 3. Cấu trúc thư mục dự án

```
LINE_CAR/
├── doc/                                 # Thư mục tài liệu kỹ thuật
│   ├── README.md                        # Mục lục tài liệu & tra cứu nhanh
│   ├── 01_TONG_QUAN_VA_CAU_TRUC.md      # Tài liệu tổng quan & cấu trúc
│   ├── 02_SO_DO_PHAN_CUNG_PINOUT.md     # Sơ đồ đấu nối phần cứng & chân pin
│   ├── 03_KIEN_TRUC_PHAN_MEM_FSM.md     # Thiết kế phần mềm, FSM & API
│   ├── 04_THUAT_TOAN_PID_VA_CAM_BIEN.md # Toán học PID & giải thuật cảm biến
│   ├── 05_HUONG_DAN_VAN_HANH_MENU.md    # Hướng dẫn thao tác nút bấm & menu OLED
│   └── 06_DE_XUAT_NANG_CAP_TOI_UU.md    # Kế hoạch tối ưu & nâng cấp tương lai
│
└── LINE_CAR/
    ├── MDK/                             # Thư mục dự án Keil uVision 5
    │   ├── car_line.uvprojx             # File project Keil MDK
    │   ├── car_line.hex                 # File hex nạp vi điều khiển
    │   ├── Listings/                    # File map, phân bổ bộ nhớ
    │   ├── Objects/                     # File object biên dịch .o
    │   └── RTE/                         # Thư viện CMSIS & Startup Device
    │
    └── USER/                            # Toàn bộ mã nguồn ứng dụng (C Source)
        ├── main.c                       # Hàm main, khởi tạo và vòng lặp chính
        ├── car_app.c / .h               # Điều phối máy trạng thái FSM (STOP, RUN, CALIB, MENU)
        ├── pid.c / .h                   # Module thuật toán PID & phân phối tốc độ bánh xe
        ├── qtr8a.c / .h                 # Driver đọc ADC, hiệu chuẩn Min/Max, tính vị trí line
        ├── tb6612.c / .h                # Driver băm xung PWM Timer 1 và điều khiển hướng động cơ
        ├── button.c / .h                # Driver lọc rung nút bấm, Remap chân JTAG PB3/PB4
        ├── oled.c / .h                  # Driver OLED SSD1306 I2C (vẽ pixel, chữ số, thanh đồ thị)
        ├── oled_font.c / .h             # Bảng mã font ký tự ASCII 8x16, 6x8
        └── delay.c / .h                 # Driver SysTick định thời millis(), microsecond
```

---

## 4. Môi trường biên dịch & Cấu hình Keil MDK

* **IDE:** Keil uVision 5.3x+
* **Compiler:** ARMCLANG Compiler V6.24 (ARM Cortex-M tối ưu hóa cao)
* **Target:** STM32F103C8 (Device Pack `Keil::STM32F1xx_DFP@2.4.1`)
* **Pre-processor Symbols:** `USE_STDPERIPH_DRIVER`, `STM32F10X_MD`
* **Chuẩn ngôn ngữ:** C99 Mode
* **Thống kê dung lượng sau biên dịch (Build Output):**
  ```
  Program Size: Code=21044 RO-data=2684 RW-data=88 ZI-data=2768
  ```
  * **Flash ROM chiếm:** $\text{Code} + \text{RO-data} + \text{RW-data} = 21044 + 2684 + 88 = 23,816\text{ Bytes}$ ($\sim 36.3\%$ trên tổng $64\text{ KB}$).
  * **SRAM chiếm:** $\text{RW-data} + \text{ZI-data} = 88 + 2768 = 2,856\text{ Bytes}$ ($\sim 13.9\%$ trên tổng $20\text{ KB}$). Trong đó `s_oled_buffer` (Framebuffer OLED) chiếm 1024 Bytes.
  * Hệ thống còn dư rất nhiều tài nguyên RAM và Flash để tích hợp thêm tính năng mới.

---

## 5. Hướng dẫn nạp chương trình (Flashing Guide)

1. **Công cụ nạp:** Mạch nạp **ST-Link V2** (hoặc J-Link / DAPLink).
2. **Cổng nạp trên xe:** Sử dụng giao tiếp **SWD (Serial Wire Debug)**:
   * Chân `GND` $\rightarrow$ `GND` xe
   * Chân `SWCLK` $\rightarrow$ Chân `PA14` của STM32
   * Chân `SWDIO` $\rightarrow$ Chân `PA13` của STM32
   * Chân `3.3V` $\rightarrow$ Chân `3.3V` xe (Lưu ý: Nếu xe đang bật pin, chỉ cần nối GND, SWCLK, SWDIO).
3. **Thao tác trong Keil MDK:**
   * Mở file `LINE_CAR/MDK/car_line.uvprojx`.
   * Bấm phím **F7** (hoặc nút *Rebuild*) để biên dịch lại chương trình.
   * Bấm phím **F8** (hoặc nút *Download*) để nạp mã máy vào vi điều khiển.
