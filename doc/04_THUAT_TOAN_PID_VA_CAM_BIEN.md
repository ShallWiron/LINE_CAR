# 04. THUẬT TOÁN ĐIỀU KHIỂN PID VÀ XỬ LÝ CẢM BIẾN

## 1. Thuật toán xử lý tín hiệu mảng Quang trở (Photoresistor / LDR Array)

### 1.1. Tối ưu phần cứng: Đọc ADC1 bằng DMA Circular + 4x Oversampling
* Hệ thống sử dụng mảng **8 cặp LED + Quang trở (LDR)** kết nối với các chân `PA0` - `PA7` (ADC1 Channel 0 - 7).
* **Đặc tính quang trở:** Cầu phân áp quang trở có trở kháng nguồn ngõ ra khá lớn ($1\text{k}\Omega \dots 50\text{k}\Omega$). Để tránh hiện tượng sụt áp khi nạp tụ lấy mẫu ADC, thời gian lấy mẫu được đặt ở mức chuẩn `ADC_SampleTime_71Cycles5`.
* **Cơ chế DMA Continuous Scan Mode:**
  * ADC1 quét tuần hoàn liên tục không ngừng nghỉ qua 8 kênh.
  * DMA1 Channel 1 tự động chuyển kết quả vào mảng tròn `s_adc_dma_buffer[32]` (gồm 4 luợt quét liên tiếp x 8 kênh).
  * Hàm `QTR8A_ReadRaw()` tính trung bình cộng 4 mẫu bằng phép dịch bit `>> 2`:
    $$V_{raw}[i] = \frac{1}{4} \sum_{k=0}^{3} \text{Buffer}[i + k \times 8]$$
  * **Thời gian xử lý của CPU:** Dưới $1\mu\text{s}$ (không có bất kỳ vòng lặp `while` chờ cờ EOC nào), hoàn toàn loại bỏ hiện tượng treo CPU hoặc trễ chu kỳ điều khiển.

---

### 1.2. Chuẩn hóa giá trị hiệu chuẩn (Calibration Normalization)
Giá trị điện áp Analog đọc từ ADC 12-bit dao động từ $0$ đến $4095$. Tùy thuộc vào ánh sáng môi trường, màu sàn và độ cao của xe, giá trị này sẽ thay đổi.
Trong quá trình hiệu chuẩn (`QTR8A_Calibrate`), hệ thống liên tục ghi lại giá trị nhỏ nhất ($V_{min}$) và lớn nhất ($V_{max}$) cho từng mắt:

$$V_{calibrated} = \frac{V_{raw} - V_{min}}{V_{max} - V_{min}} \times 1000$$

* Kết quả trả về luôn nằm chuẩn xác trong thang đo $[0, 1000]$ cho từng mắt cảm biến.
* Với chế độ line trắng trên nền đen: Giá trị được đảo ngược tự động $V_{calibrated} = 1000 - V_{calibrated}$.

---

### 1.3. Thuật toán xác định tâm line bằng trọng số (Centroid Method)
Để đạt độ mịn và độ chính xác tới từng phần mười milimet (sub-sensor resolution) thay vì chỉ biết vạch line đang ở mắt nào, hệ thống tính toán vị trí trọng tâm quang học:

$$\text{Position} = \frac{\sum_{i=0}^{7} \left( V_i \times i \times 1000 \right)}{\sum_{i=0}^{7} V_i}$$

* Mắt 0 (ngoài cùng bên trái) có tọa độ: $0$
* Mắt 3 và Mắt 4 (trung tâm thanh cảm biến) có tọa độ trung bình: **$3500$**
* Mắt 7 (ngoài cùng bên phải) có tọa độ: $7000$

```
 Mắt 0    Mắt 1    Mắt 2    Mắt 3    Mắt 4    Mắt 5    Mắt 6    Mắt 7
   |--------|--------|--------|--------|--------|--------|--------|
   0       1000     2000     3000     4000     5000     6000     7000
                              ^ TÂM CHUẨN = 3500
```

#### Bộ lọc nhiễu nền và ngưỡng nhận diện:
* **Ngưỡng nhận diện trên line:** Nếu $V_i > 200$ (tương đương $20\%$ độ tương phản tối đa), xác nhận ít nhất có 1 mắt đang nhìn thấy vạch.
* **Ngưỡng lọc nhiễu phản xạ:** Chỉ những mắt có $V_i > 50$ mới được đưa vào công thức cộng dồn trọng số để loại bỏ ánh sáng tán xạ từ môi trường.

---

### 1.4. Cơ chế cứu hộ khi mất vạch line (Line Loss Recovery)
Khi xe di chuyển với vận tốc cao hoặc vượt qua khe đứt nét / góc cua gắt, toàn bộ 8 mắt có thể tạm thời không thấy line (`on_line == 0` hoặc $\sum V_i = 0$).
Hệ thống sử dụng bộ nhớ vị trí cuối cùng (`s_last_position`) phân loại 3 trường hợp:

```c
if (!on_line || sum == 0)
{
    if (s_last_position < 2500)
    {
        return 0;      /* Lệch hẳn sang bên trái -> bẻ lái tối đa sang trái */
    }
    else if (s_last_position > 4500)
    {
        return 7000;   /* Lệch hẳn sang bên phải -> bẻ lái tối đa sang phải */
    }
    else
    {
        return 3500;   /* Vừa mất ở vùng trung tâm -> tiếp tục giữ thẳng để lao qua khe đứt line */
    }
}
```
* **Vùng trung tâm ($[2500, 4500]$):** Nếu vạch line vừa biến mất ngay giữa xe (ví dụ: vạch đứt quãng, vạch giao nhau), xe duy trì chạy thẳng hướng $3500$ thay vì giật ngoặc sang hai bên.
* **Vùng biên cực ($<2500$ hoặc $>4500$):** Xe ép vị trí về $0$ hoặc $7000$ để kích hoạt cơ chế bẻ lái hoặc quay tại chỗ tìm lại line.

---

## 2. Giải thuật điều khiển PID và Phân phối lực kéo

### 2.1. Công thức tính sai số và tín hiệu điều khiển
Mục tiêu là giữ vị trí xe trùng với tâm cảm biến ($3500$):

$$\text{Error: } e(t) = \text{Position}(t) - 3500$$

* $e(t) = 0$: Xe đang ở chính giữa line.
* $e(t) > 0$: Xe đang bị lệch sang **bên phải** vạch line (cần bẻ sang trái).
* $e(t) < 0$: Xe đang bị lệch sang **bên trái** vạch line (cần bẻ sang phải).

Tín hiệu điều khiển đầu ra $u(t)$:

$$u(t) = K_p \cdot e(t) + K_i \cdot \int_{0}^{t} e(\tau) d\tau + K_d \cdot \frac{e(t) - e(t-1)}{\Delta t}$$

* **Khâu Tỉ lệ ($P = K_p \cdot e$):** Phản ứng tỷ lệ thuận với độ lệch hiện tại.
* **Khâu Tích phân ($I = K_i \cdot \sum e$):** Triệt tiêu sai số xác lập. Kèm bộ chặn **Anti-windup** ($\pm 10000$) và hàm `PID_ResetIntegral` để xả toàn bộ tích phân ngay sau khi thoát cua vuông $90^\circ$, tránh hiện tượng giật văng đuôi xe (post-turn snap).
* **Khâu Vi phân ($D = K_d \cdot (e - e_{last})$):** Dự đoán xu hướng thay đổi độ lệch, dập tắt dao động lắc lư của xe khi bám đường thẳng.

---

### 2.2. Phân phối vận tốc và Giảm tốc động khi vào cua (Dynamic Corner Deceleration)
Tín hiệu $u(t)$ sau khi giới hạn $[-100, 100]$ được dùng để điều chỉnh vận tốc 2 bánh. Để triệt tiêu quán tính tiến làm xe văng khỏi vạch khi bẻ lái gấp, thuật toán tự động giảm tốc độ cơ sở `eff_base`:

```c
int16_t eff_base = pid->base_speed;
int16_t abs_out = (pid_output < 0) ? -pid_output : pid_output;
if (abs_out > 35)
{
    /* Giảm dần tốc độ cơ sở khi sai số lớn để giảm quán tính văng xe */
    int16_t decel = (int16_t)(((int32_t)(abs_out - 35) * pid->base_speed) / 100);
    eff_base -= decel;
    if (eff_base < 12) eff_base = 12;
}

int16_t left = eff_base - pid_output;
int16_t right = eff_base + pid_output;
```

* **Đường thẳng & cua nhẹ ($|u| \le 35$):** Duy trì $100\%$ tốc độ cơ sở (`base_speed`).
* **Cua gắt ($|u| > 35$):** Tốc độ tịnh tiến tự động ghìm lại, bánh trong được phép quay lùi sâu tới `min_speed = -55%` sinh mô-men xoay thân xe cực mạnh mà không bị trôi về phía trước.

---

## 3. Thuật toán xử lý chuyên biệt Góc Cua Vuông 90° (Sharp 90° Turn)

### 3.1. Bản chất vật lý vì sao PID thuần túy thất bại ở góc 90°
1. **Bán kính quay bằng 0:** Khác với khúc cua tròn ($R > 20\text{cm}$), góc vuông $90^\circ$ đòi hỏi xe phải đổi hướng gần như vuông góc tại chỗ ($R \approx 0$).
2. **Quán tính tịnh tiến:** Nếu chỉ dùng vi sai thông thường, vận tốc trung bình về phía trước $(V_L + V_R)/2$ vẫn dương ($>20\%$). Xe tiếp tục lao thẳng vượt qua góc vuông trước khi thân xe kịp xoay đủ $90^\circ$.
3. **Mắt cảm biến rơi vào vùng mù:** Thanh cảm biến bố trí phía trước bánh xe, khi xe lao thẳng qua góc, toàn bộ 8 mắt vọt ra ngoài nền trắng làm mất vạch hoàn toàn.

### 3.2. Máy trạng thái 3 pha (3-Phase Corner State Machine)
Hệ thống sử dụng cơ chế kết hợp: **Nhận diện dạng vạch $\rightarrow$ Phanh cưỡng bức $\rightarrow$ Quay tại chỗ (Pivot Turn) $\rightarrow$ Khôi phục PID**:

```
[Chạy PID bình thường]
         │
         ▼ Phát hiện mẫu góc 90° (QTR8A_DetectCorner) hoặc Mất line ở biên cực
[Pha 1: CORNER_MANEUVER_BRAKE (15ms)]
  - Kích hoạt Motor_Brake() (Short Brake TB6612)
  - Triệt tiêu 100% quán tính tiến của xe ngay tại đỉnh góc cua
         │
         ▼ Sau 15ms
[Pha 2: CORNER_MANEUVER_PIVOT]
  - Bánh trong quay lùi (-55%), bánh ngoài quay tiến (+55%)
  - Trục quay nằm chính giữa 2 bánh, vận tốc tiến bằng 0
  - Cảm biến quét hình nan quạt đón đầu vạch vuông góc
         │
         ▼ Mắt trung tâm (Mắt 3 hoặc 4 > 380) bắt lại được line (sau tối thiểu 40ms)
[Pha 3: HOÀN TẤT & PHỤC HỒI]
  - Thoát Corner Maneuver về IDLE
  - Gọi PID_Reset(&s_pid) xả sạch tích phân I tích tụ
  - PID tiếp tục bám line mượt mà trên đoạn đường mới!
```

### 3.3. Dấu hiệu nhận diện góc vuông 90° (`QTR8A_DetectCorner`)
* **Góc 90° Trái:**
  * Mắt ngoài cùng trái (Mắt 0, 1) bắt vạch rất đậm ($>450$).
  * Mắt ngoài cùng phải (Mắt 6, 7) hoàn toàn ngoài nền trắng ($<250$).
  * Bất đối xứng cực lớn: $\sum V_{0..3} > 1400$ và $\sum V_{4..7} < 600$.
  * Không phải vạch ngang ngã tư (`!QTR8A_IsCrossLine`).
* **Góc 90° Phải:** Tương tự đối xứng qua bên phải.
* **Chỉ báo trực quan:** Màn hình OLED hiển thị banner `<= 90L TURN <=` hoặc `=> 90R TURN =>` ngay khi kích hoạt pha xử lý.
