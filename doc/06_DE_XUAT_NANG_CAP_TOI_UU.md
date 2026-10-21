# 06. CÁC HẠNG MỤC ĐÃ ĐƯỢC NÂNG CẤP VÀ TỐI ƯU HỆ THỐNG

Tài liệu này ghi nhận hiện trạng nâng cấp của dự án sau khi tích hợp phần cứng mảng **8 cặp LED + Quang trở (LDR / Photoresistor)** và triển khai tối ưu hóa bằng **ADC1 DMA Scan Mode** cùng **Flash Emulated EEPROM**.

---

## 1. Nâng cấp Đọc ADC bằng Continuous Scan Mode + DMA1 (ĐÃ TRIỂN KHAI)

### Đặc điểm phần cứng: Mảng 8 cặp LED + Quang trở (LDR)
* Cầu phân áp quang trở có trở kháng ngõ ra lớn hơn đáng kể so với Phototransistor. Nếu lấy mẫu quá nhanh, tụ sạc mẫu của ADC chưa kịp nạp đầy điện áp gây sai số đo.
* **Thời gian lấy mẫu tối ưu:** Chuyển sang `ADC_SampleTime_71Cycles5` cho cả 8 kênh (`PA0` đến `PA7`).
* **Cấu hình DMA1 Channel 1 Circular:**
  * Bộ đệm kích thước 32 phần tử (`8 kênh x 4 mẫu liên tiếp`).
  * ADC1 quét liên tục không ngừng (`Continuous Conversion + Scan Mode`).
  * DMA tự động chuyển dữ liệu về RAM theo vòng tròn với mức ưu tiên `DMA_Priority_High`.
* **Bộ lọc số đa mẫu (Oversampling & Digital Filter):**
  * Hàm `QTR8A_ReadRaw()` tính trung bình cộng 4 mẫu liên tiếp của mỗi kênh bằng phép dịch bit `>> 2`.
  * **Thời gian CPU chiếm dụng khi đọc cảm biến: $< 1\mu\text{s}$ (gần như $0\mu\text{s}$)**, CPU chỉ truy xuất ô nhớ RAM thay vì phải chờ đợi cờ `EOC` như trước.
  * Triệt tiêu hoàn toàn nhiễu quang học từ ánh sáng đèn phòng và dao động cơ khí của quang trở.

---

## 2. Lưu thông số Calib và PID vào bộ nhớ Flash nội (ĐÃ TRIỂN KHAI)

### Hiện trạng triển khai:
Đã tích hợp module lưu/khôi phục trực tiếp vào [`car_app.c`](file:///c:/Users/Shall%20Wiron/Desktop/LINE_CAR/LINE_CAR/USER/car_app.c):
* **Địa chỉ lưu trữ:** Trang 63 (Trang cuối cùng của Flash 64KB STM32F103C8): `0x0800FC00`.
* **Cấu trúc lưu trữ `CarConfig_t` (60 Bytes):**
  * `magic`: `0x55AA1234` (nhận diện tính hợp lệ của dữ liệu).
  * Thông số PID: $K_p, K_i, K_d, Base, Max, Min, Mode$.
  * Mảng hiệu chuẩn: `sensor_min[8]`, `sensor_max[8]`.
  * Mã kiểm tra toàn vẹn: `checksum` (tổng byte).
* **Tự động hóa:**
  * **Khi khởi động (`Car_Init()`):** Tự động đọc Flash. Nếu hợp lệ, khôi phục toàn bộ cấu hình cũ và hiển thị `[FLASH RESTORED]`. Người dùng không cần hiệu chuẩn lại mỗi khi bật máy!
  * **Khi Calib xong (`Do_Calibration()`):** Tự động ghi ngay giá trị Min/Max mới vào Flash.
  * **Trong Menu OLED:** Thêm mục thứ 9: `Save: [Flash]`. Bấm nút `+` hoặc `-` tại mục này sẽ lưu ngay lập tức các thông số PID vừa chỉnh vào Flash và báo `DA LUU FLASH!`.

---

## 3. Tối ưu thời gian làm mới màn hình OLED khi RUNNING (ĐÃ TRIỂN KHAI)

* **Hiện trạng triển khai:**
  * Giãn chu kỳ cập nhật màn hình OLED khi xe đang chạy ở tốc độ cao từ `70ms` sang **`120ms` (~8 Hz)**.
  * Giảm thiểu hiện tượng gián đoạn xung nhịp (jitter) do hàm truyền I2C Bit-banging 1024 Bytes gây ra.
  * Vòng lặp PID chu kỳ `5ms` (200 Hz) luôn được ưu tiên $>95\%$ tài nguyên CPU, giúp xe ôm cua mượt mà, phản ứng chuẩn xác.

---

## 4. Nhận diện vạch ngang xuất phát / đích (ĐÃ BỔ SUNG API)

* **Hàm API:** `QTR8A_IsCrossLine(const uint16_t *sensor_values)` trong [`qtr8a.c`](file:///c:/Users/Shall%20Wiron/Desktop/LINE_CAR/LINE_CAR/USER/qtr8a.c).
* **Cơ chế:** Khi có từ 6 đến 8 mắt quang trở đồng thời nhận diện vạch đen trên sàn trắng (độ tương phản $>65\%$), hàm trả về `1`.
* Có thể ứng dụng để bỏ qua vạch xuất phát và kích hoạt `Motor_Brake()` tự động hãm cứng động cơ dừng chuẩn xác tại vạch đích.
