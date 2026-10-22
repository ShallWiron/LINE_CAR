# 02. SƠ ĐỒ KẾT NỐI PHẦN CỨNG VÀ BẢNG CHÂN PINOUT

## 1. Bảng phân bổ chân kết nối vi điều khiển (Complete Pinout Table)

| STT | Chân STM32 | Tên ngoại vi trên Chip | Chức năng trên Xe | Thiết bị ngoại vi kết nối | Cấu hình trong Code |
| :---: | :---: | :--- | :--- | :--- | :--- |
| 1 | **PA0** | ADC1_IN0 | Mắt cảm biến 1 (Ngoài cùng trái) | QTR-8A Pin 1 | `GPIO_Mode_AIN` |
| 2 | **PA1** | ADC1_IN1 | Mắt cảm biến 2 | QTR-8A Pin 2 | `GPIO_Mode_AIN` |
| 3 | **PA2** | ADC1_IN2 | Mắt cảm biến 3 | QTR-8A Pin 3 | `GPIO_Mode_AIN` |
| 4 | **PA3** | ADC1_IN3 | Mắt cảm biến 4 | QTR-8A Pin 4 | `GPIO_Mode_AIN` |
| 5 | **PA4** | ADC1_IN4 | Mắt cảm biến 5 | QTR-8A Pin 5 | `GPIO_Mode_AIN` |
| 6 | **PA5** | ADC1_IN5 | Mắt cảm biến 6 | QTR-8A Pin 6 | `GPIO_Mode_AIN` |
| 7 | **PA6** | ADC1_IN6 | Mắt cảm biến 7 | QTR-8A Pin 7 | `GPIO_Mode_AIN` |
| 8 | **PA7** | ADC1_IN7 | Mắt cảm biến 8 (Ngoài cùng phải) | QTR-8A Pin 8 | `GPIO_Mode_AIN` |
| 9 | **PA8** | TIM1_CH1 | Xung PWM điều khiển tốc độ Bánh Phải | TB6612 Chân `PWMB` | `GPIO_Mode_AF_PP` (1 kHz) |
| 10 | **PA9** | TIM1_CH2 | Xung PWM điều khiển tốc độ Bánh Trái | TB6612 Chân `PWMA` | `GPIO_Mode_AF_PP` (1 kHz) |
| 11 | **PB3** | GPIO_Pin_3 (JTDO) | Nút nhấn `BTN_RUN` (Chạy / Dừng / OK) | Nút bấm 1 (nối GND) | `GPIO_Mode_IPU` (Remap JTAG-Disable) |
| 12 | **PB4** | GPIO_Pin_4 (JNTRST)| Nút nhấn `BTN_MODE` (Calib / Giảm `-`) | Nút bấm 2 (nối GND) | `GPIO_Mode_IPU` (Remap JTAG-Disable) |
| 13 | **PB5** | GPIO_Pin_5 | Nút nhấn `BTN_UP` (Menu / Tăng `+`) | Nút bấm 3 (nối GND) | `GPIO_Mode_IPU` |
| 14 | **PB6** | GPIO_Pin_6 | Nút nhấn `BTN_DOWN` (Menu / Xuống `v`)| Nút bấm 4 (nối GND) | `GPIO_Mode_IPU` |
| 15 | **PB10** | GPIO_Pin_10 | Tín hiệu xung Clock I2C (SCL) | OLED SSD1306 `SCL` | `GPIO_Mode_Out_PP` (Bit-bang) |
| 16 | **PB11** | GPIO_Pin_11 | Tín hiệu dữ liệu I2C (SDA) | OLED SSD1306 `SDA` | `GPIO_Mode_Out_OD` / Read |
| 17 | **PB12** | GPIO_Pin_12 | Tín hiệu hướng logic AIN2 | TB6612 Chân `AIN2` | `GPIO_Mode_Out_PP` |
| 18 | **PB13** | GPIO_Pin_13 | Tín hiệu hướng logic AIN1 | TB6612 Chân `AIN1` | `GPIO_Mode_Out_PP` |
| 19 | **PB14** | GPIO_Pin_14 | Tín hiệu hướng logic BIN1 | TB6612 Chân `BIN1` | `GPIO_Mode_Out_PP` |
| 20 | **PB15** | GPIO_Pin_15 | Tín hiệu hướng logic BIN2 | TB6612 Chân `BIN2` | `GPIO_Mode_Out_PP` |
| 21 | **PA13** | JTMS / SWDIO | Nạp và Debug code | Mạch nạp ST-Link | Giữ nguyên SWD |
| 22 | **PA14** | JTCK / SWCLK | Nạp và Debug code | Mạch nạp ST-Link | Giữ nguyên SWD |

---

## 2. Phân tích chi tiết từng khối phần cứng

### 2.1. Khối Driver động cơ TB6612FNG & Động cơ tải nặng JGB37-520
TB6612FNG là mạch cầu H kép sử dụng MOSFET nội trở thấp (ít sinh nhiệt và sụt áp thấp hơn rất nhiều so với L298N cổ điển).
* **Chân nguồn:**
  * `VM`: Nối trực tiếp nguồn Pin (7.4V - 11.1V). **Bắt buộc lắp thêm tụ hóa $470\mu\text{F} - 1000\mu\text{F}$ / 16V hoặc 25V** sát chân VM để dập gai áp phản hồi lớn từ cuộn dây động cơ JGB37-520.
  * `VCC`: Nối nguồn logic $5\text{V}$ (hoặc $3.3\text{V}$).
  * `GND`: Nối chung Mass với STM32 và nguồn Pin.
  * `STBY`: Nối cố định lên $VCC$ ($3.3\text{V}$ hoặc $5\text{V}$) để kích hoạt driver hoạt động.
* **LƯU Ý ĐẶC BIỆT KHI DÙNG VỚI ĐỘNG CƠ JGB37-520:**
  * Động cơ JGB37-520 có dòng kẹt (stall current) lên tới $1.5\text{A} - 2.5\text{A}$, vượt mức dòng liên tục $1.2\text{A}$ của TB6612.
  * **Bắt buộc dán tản nhiệt nhôm nhỏ** lên lưng chip TB6612FNG để tránh bị ngắt nhiệt (Thermal Shutdown) khi vào cua $90^\circ$ hoặc đảo chiều gấp.
  * Xem phân tích chi tiết tại file tài liệu: `07_PHAN_TICH_DONG_CO_JGB37_VA_CAC_TRUONG_HOP_VAN_DE.md`.
* **Bảng chân lý điều khiển hướng động cơ:**
  | IN1 | IN2 | PWM | Trạng thái động cơ |
  | :---: | :---: | :---: | :--- |
  | `H` | `L` | Xung | Quay thuận (Forward) |
  | `L` | `H` | Xung | Quay nghịch (Reverse) |
  | `L` | `L` | 0 | Phanh trôi (Coast / Float - Tự do) |
  | `H` | `H` | 0 | Phanh hãm chủ động (Short Brake) |

### 2.2. Khối cảm biến dò vạch Pololu QTR-8A
* Bao gồm 8 cặp LED phát hồng ngoại và Phototransistor thu. Điện áp ngõ ra là mức Analog ($0\text{V} \sim 3.3\text{V}$) phụ thuộc vào độ hấp thụ ánh sáng của bề mặt:
  * Khi gặp bề mặt đen: Hấp thụ tia hồng ngoại $\rightarrow$ Ngõ ra tiệm cận điện áp cao ($VCC$).
  * Khi gặp bề mặt trắng: Phản xạ mạnh tia hồng ngoại $\rightarrow$ Ngõ ra hạ xuống tiệm cận $0\text{V}$.
* Các mắt bố trí cách đều 9.525 mm. Khoảng cách lắp đặt tối ưu từ mắt cảm biến tới mặt sàn thi đấu là **3 mm - 5 mm**.

### 2.3. Khối Nút bấm & Kỹ thuật Remap JTAG và Tự nhận diện cực tính (Quan trọng)
* Mặc định sau Reset phần cứng của STM32F103, các chân `PB3`, `PB4`, `PA15` được vi điều khiển gán cho giao diện nạp 5-dây JTAG.
* Trong hàm `Button_Init()`, hệ thống kích hoạt khối AFIO và vô hiệu hóa JTAG:
  ```c
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
  AFIO->MAPR = (AFIO->MAPR & ~((uint32_t)0x07000000)) | ((uint32_t)0x02000000);
  ```
  * Cấu hình này ngắt kết nối JTAG để giải phóng `PB3` và `PB4` làm GPIO Input bình thường.
  * Giữ nguyên giao tiếp SW-DP (`PA13`/`PA14`), đảm bảo nạp code và debug qua ST-Link không bao giờ bị khóa chip.
* **Cơ chế Tự động nhận diện cực tính nút bấm (Auto-polarity Detection):**
  * Tại thời điểm khởi động (`Button_Init()`), hệ thống đo điện áp tĩnh ban đầu của từng chân nút bấm và lưu vào biến `idle_level`.
  * Dù phần cứng đấu nút bấm kéo xuống GND (Active-LOW) hay đấu kéo lên VCC (Active-HIGH), hệ thống đều tự động phát hiện khi trạng thái pin bị đảo chiều $\rightarrow$ Hoạt động tương thích $100\%$ với mọi thiết kế mạch!
* **Hiển thị trực quan trạng thái phím trên OLED:**
  * Màn hình chờ hiển thị trực tiếp `BTN: [b1 b2 b3 b4]` (1 = đang nhấn, 0 = nhả). Nhấn nút nào con số đó lập tức chuyển sang `1`, giúp chẩn đoán tức thì tình trạng kết nối vật lý của từng nút bấm.

### 2.4. Khối màn hình OLED SSD1306 (I2C)
* Được điều khiển theo phương pháp mô phỏng I2C (Bit-banging) trên `PB10` (SCL) và `PB11` (SDA).
* Lý do sử dụng Bit-bang thay vì Hardware I2C của STM32F1: Khối Hardware I2C1/I2C2 trên dòng STM32F103 rất dễ bị kẹt cờ `BUSY` vĩnh viễn khi có xung nhiễu từ động cơ gây ra sụt áp trên đường bus. Bit-bang hoàn toàn miễn nhiễm với lỗi này và chủ động reset bus được bất cứ lúc nào.

---

## 3. Lưu ý sống còn về đường nguồn và chống nhiễu (EMC/EMI)
1. **Phân tách nguồn:**
   * Nguồn Pin LiPo $\rightarrow$ Cấp trực tiếp cho chân `VM` của Driver động cơ.
   * Nguồn Pin LiPo $\rightarrow$ Qua mạch Buck (LM2596 / MP1584 / AMS1117) hạ áp xuống $5\text{V}$ và $3.3\text{V}$ sạch cấp cho STM32, OLED và Cảm biến QTR-8A.
2. **Nối chung GND:** Toàn bộ GND của Pin, Driver, STM32, Cảm biến và OLED bắt buộc phải nối chung tại một điểm (Star Ground Topology) để tránh lệch điện áp tham chiếu đo ADC.
3. **Chống nhiễu động cơ:**
   * Hàn 3 tụ gốm $104$ ($0.1\mu\text{F}$) trên từng động cơ DC: 1 tụ giữa 2 cực động cơ, 2 tụ nối từ từng cực vào vỏ sắt của động cơ.
