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
Khi xe di chuyển với vận tốc cao hoặc vượt qua góc cua gắt, toàn bộ 8 mắt có thể tạm thời không thấy line (`on_line == 0` hoặc $\sum V_i = 0$).
Hệ thống sử dụng bộ nhớ vị trí cuối cùng (`s_last_position`) phân loại bẻ lái khẩn cấp:

```c
if (!on_line || sum == 0)
{
    if (s_last_position < 3500)
    {
        return 0;      /* Đã từng ở nửa trái -> bẻ lái tối đa sang trái */
    }
    else
    {
        return 7000;   /* Đã từng ở nửa phải -> bẻ lái tối đa sang phải */
    }
}
```
* **Không duy trì ảo tưởng trung tâm:** Phiên bản cũ trả về `3500` khi mất line ở vùng giữa làm xe tiếp tục lao thẳng ra ngoài sàn đấu khi vừa chớm vượt qua góc cua $90^\circ$. Phiên bản mới lập tức ép vị trí về cực trị $0$ hoặc $7000$ tùy theo nửa phía mà xe vừa rời khỏi, giúp máy trạng thái góc cua lập tức can thiệp phanh và quay tại chỗ.

---

## 2. Giải thuật điều khiển PID và Phân phối lực kéo

### 2.1. Công thức tính sai số và tín hiệu điều khiển
Mục tiêu là giữ vị trí xe trùng với tâm cảm biến ($3500$):

$$\text{Error: } e(t) = \text{Position}(t) - 3500$$

* $e(t) = 0$: Xe đang ở chính giữa line.
* $e(t) > 0$: Vạch line nằm lệch về phía mắt 4..7 (bên phải).
* $e(t) < 0$: Vạch line nằm lệch về phía mắt 0..3 (bên trái).

Tín hiệu điều khiển đầu ra $u(t)$:

$$u(t) = K_p \cdot e(t) + K_i \cdot \int_{0}^{t} e(\tau) d\tau + K_d \cdot \frac{e(t) - e(t-1)}{\Delta t}$$

* **Khâu Tỉ lệ ($P = K_p \cdot e$):** Phản ứng tỷ lệ thuận với độ lệch hiện tại.
* **Khâu Tích phân ($I = K_i \cdot \sum e$):** Triệt tiêu sai số xác lập. Kèm bộ chặn **Anti-windup** ($\pm 10000$) và hàm `PID_ResetIntegral` để xả toàn bộ tích phân ngay sau khi thoát cua vuông $90^\circ$, tránh hiện tượng giật văng đuôi xe (post-turn snap).
* **Khâu Vi phân ($D = K_d \cdot (e - e_{last})$):** Dự đoán xu hướng thay đổi độ lệch, dập tắt dao động lắc lư của xe khi bám đường thẳng.

---

### 2.2. Phân phối vận tốc, Giảm tốc động & Bù vùng chết ma sát (Friction Deadband)
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

/* Bộ lọc chống kẹt vùng chết ma sát hộp số JGB37-520 (Deadband Filter >= 20%) */
#define JGB37_MIN_DRIVE_PWM  20
if (left > 0 && left < JGB37_MIN_DRIVE_PWM) left = JGB37_MIN_DRIVE_PWM;
else if (left < 0 && left > -JGB37_MIN_DRIVE_PWM) left = -JGB37_MIN_DRIVE_PWM;

if (right > 0 && right < JGB37_MIN_DRIVE_PWM) right = JGB37_MIN_DRIVE_PWM;
else if (right < 0 && right > -JGB37_MIN_DRIVE_PWM) right = -JGB37_MIN_DRIVE_PWM;
```

* **Bù vùng chết ma sát $\ge 20\%$:** Động cơ kim loại JGB37-520 với hộp số giảm tốc có ma sát tĩnh rất lớn. Nếu cấp PWM nhỏ ($5\% \dots 15\%$), rotor bị kẹt cứng (stall), dòng điện tăng vọt gây nóng và ngắt nhiệt bảo vệ TB6612 (hiện tượng xe chỉ chạy 1 bánh). Bộ lọc đảm bảo mọi lệnh quay đều có xung tối thiểu $20\%$ để bánh xe quay ngay lập tức.
* **Đường thẳng & cua nhẹ ($|u| \le 35$):** Duy trì $100\%$ tốc độ cơ sở (`base_speed`).
* **Cua gắt ($|u| > 35$):** Tốc độ tịnh tiến tự động ghìm lại, bánh trong được phép quay lùi sâu tới `min_speed` (tinh chỉnh $0\% \dots -80\%$ qua Menu OLED) sinh mô-men xoay thân xe cực mạnh mà không bị trôi về phía trước.

---

## 3. Thuật toán xử lý chuyên biệt Góc Cua Vuông 90° (Sharp 90° Turn)

### 3.1. Bản chất vật lý vì sao PID thuần túy thất bại ở góc 90°
1. **Bán kính quay bằng 0:** Khác với khúc cua tròn ($R > 20\text{cm}$), góc vuông $90^\circ$ đòi hỏi xe phải đổi hướng gần như vuông góc tại chỗ ($R \approx 0$).
2. **Quán tính tịnh tiến của xe nặng JGB37 (~1kg):** Nếu chỉ dùng vi sai thông thường, vận tốc trung bình về phía trước $(V_L + V_R)/2$ vẫn dương. Quán tính đẩy cả khối xe lao thẳng vượt qua góc vuông trước khi thân xe kịp xoay đủ $90^\circ$.
3. **Mắt cảm biến rơi vào vùng mù:** Thanh cảm biến bố trí nhô ra phía trước bánh xe, khi xe lao thẳng qua góc, toàn bộ 8 mắt vọt ra ngoài nền trắng làm mất vạch hoàn toàn.

### 3.2. Máy trạng thái 4 pha xử lý góc vuông (4-Phase Corner State Machine)
Do cảm biến nằm nhô ra trước trục bánh xe ($5\text{cm} \dots 8\text{cm}$), nếu phanh xoay ngay khi cảm biến vừa thấy góc thì trục bánh xe vẫn chưa tới góc $\rightarrow$ xe xoay sớm và chém vào trong văng ra ngoài.
Hệ thống sử dụng cơ chế 4 pha hoàn chỉnh: **Nhận diện $\rightarrow$ Trườn tới (Advance) $\rightarrow$ Phanh cưỡng bức (Brake) $\rightarrow$ Quay tại chỗ (Pivot) $\rightarrow$ Khôi phục PID**:

```
[Chạy PID bình thường]
         │
         ▼ Phát hiện mẫu góc 90° (QTR8A_DetectCorner) hoặc Mất line ở biên cực (pos <= 2000 hoặc >= 5000)
[Pha 1: CORNER_MANEUVER_ADVANCE (Mặc định 70ms, chỉnh trong Menu 0..200ms)]
  - 2 bánh tiếp tục tiến thẳng ổn định với tốc độ base_speed
  - Đưa trục bánh xe tiến tới đúng đỉnh góc vuông của đường đua
         │
         ▼ Sau s_corner_advance_ms
[Pha 2: CORNER_MANEUVER_BRAKE (25ms - 35ms)]
  - Kích hoạt Motor_Brake() (Short Brake TB6612: nối tắt cuộn dây động cơ)
  - Triệt tiêu 100% quán tính tiến của xe nặng JGB37 ngay tại đỉnh góc vuông
         │
         ▼ Sau 25ms - 35ms (tùy chỉnh theo mức Phanh trong Menu)
[Pha 3: CORNER_MANEUVER_PIVOT]
  - Bánh trong quay lùi (-55% .. -75%), bánh ngoài quay tiến (+55% .. +75%)
  - Trục quay nằm chính giữa 2 bánh ngay tại đỉnh góc, vận tốc tiến bằng 0
  - Quay mù tối thiểu 120ms (Blind Delay) để vượt qua vạch cũ và tạo đà xoay
         │
         ▼ Sau 120ms VÀ Mắt trung tâm (Mắt 2, 3, 4 hoặc 5 > 280) bắt lại được line mới
[Pha 4: HOÀN TẤT & PHỤC HỒI]
  - Thoát Corner Maneuver về IDLE
  - Gọi PID_Reset(&s_pid) xả sạch tích phân I tích tụ
  - PID tiếp tục bám line mượt mà trên đoạn đường mới!
```

### 3.3. Dấu hiệu nhận diện góc vuông 90° linh hoạt (`QTR8A_DetectCorner`)
* **Góc 90° Trái:**
  * Mắt ngoài cùng trái (Mắt 0 hoặc 1) bắt vạch ($>300$).
  * Mắt ngoài cùng phải (Mắt 7) không phải vạch đậm ($<450$).
  * Tỉ lệ bất đối xứng vượt trội: $\sum V_{0..3} > 800$ và lớn hơn gấp đôi $\sum V_{4..7} \times 2$.
  * Không phải vạch ngang ngã tư (`!QTR8A_IsCrossLine`).
* **Góc 90° Phải:** Tương tự đối xứng qua bên phải.
* **Quy ước chiều quay động cơ đồng bộ với PID:**
  * Cua Trái 90°: `speed_left = +p_spd`, `speed_right = -p_spd`.
  * Cua Phải 90°: `speed_left = -p_spd`, `speed_right = +p_spd`.
* **Chỉ báo trực quan:** Màn hình OLED hiển thị banner `<= 90L TURN <=` hoặc `=> 90R TURN =>` ngay khi kích hoạt pha xử lý.
