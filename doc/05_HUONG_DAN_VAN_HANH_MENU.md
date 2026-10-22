# 05. HƯỚNG DẪN VẬN HÀNH VÀ MENU CÀI ĐẶT TRÊN XE

## 1. Giao diện Màn hình Chính (`STOPPED`) - Thuần thông tin xe

Theo đúng thiết kế tối giản, màn hình chính **chỉ hiển thị thông số hoạt động của xe**, toàn bộ cài đặt được dồn vào Menu:

```
+--------------------------------+
|         LINE FOLLOWER          | <- Tiêu đề xe to rõ (Font 16)
|   Pos: 3500       Err:   +0    | <- Vị trí cảm biến và Sai lệch tâm
|   Spd:  45%       Line: Den    | <- Tốc độ đặt và Chế độ vạch màu
|         B1:CHAY   B2:MENU      | <- 2 phím thao tác chính
|     [ █   █   █ █ █   █   █ ]  | <- Đồ thị 8 mắt quang trở soi vạch line
+--------------------------------+
```

* **Nút 1 (`PB3`):** Bắt đầu chạy xe (`RUN`).
* **Nút 2 (`PB4`):** Mở Menu cài đặt (`MENU`).

---

## 1.2. Giao diện Khi Xe Đang Chạy (`RUNNING`) - Giám sát Chi tiết Từng Bánh

Khi xe đang chạy, màn hình hiển thị trực quan toàn diện trạng thái hoạt động của 2 động cơ:

```
+--------------------------------+
|         = RUNNING =            | <- Banner chữ to (hoặc <= 90L TURN <=)
| L:+45 TIEN        R:+45 TIEN   | <- Tốc độ & Chiều quay từng bánh xe
| L ──────────────■──────────── R| <- Thước đo độ lệch tâm vạch line
| Err:   +50        Line: Den    | <- Sai số lệch tâm và màu line
|      (B1: DUNG KHAN CAP)       | <- Nút bấm dừng khẩn cấp
| [████████░░░░]  [████████░░░░] | <- 2 thanh bar công suất PWM động cơ L & R
+--------------------------------+
```

* **Trạng thái từng bánh xe:**
  * **`TIEN`:** Bánh xe đang quay tiến ($PWM > 0$).
  * **`LUI `:** Bánh xe đang quay lùi ($PWM < 0$, ví dụ khi đang ôm cua hoặc quay vuông $90^\circ$).
  * **`DUNG`:** Bánh xe dừng tự do ($PWM = 0$).
  * **`PHAN`:** Bánh xe đang chịu phanh hãm cứng (`Motor_Brake()`).
* **2 Thanh bar đồ họa ở đáy màn hình:** Thể hiện độ rộng xung PWM thực tế đang cấp cho từng kênh driver. Nếu 1 bên thanh bar rỗng mà bánh không quay, người dùng biết ngay bánh đó đang nhận $0\%$ PWM; ngược lại nếu thanh bar đầy mà bánh đứng im, đó là dấu hiệu chập chờn dây hoặc TB6612 ngắt nhiệt!

---

## 2. Giao diện Menu Cài đặt tập trung (`MENU`)

Toàn bộ các tham số cấu hình của xe được dồn gọn vào Menu gồm 9 mục trực quan:

```
+--------------------------------+
|      === MENU CAI DAT ===      |
|   3.Kd:      0.450             |
|   4.Phanh:    50%              |
| > 5.Truon:    70ms             | <- Thời gian trườn tới đỉnh góc vuông 90°
|   6.Line:    Den               |
|   B4:v  B3:+  B2:-  B1:OK      |
+--------------------------------+
```

### Danh sách 9 mục cài đặt trong Menu:
1. **`1. Toc do:`** Tốc độ cơ sở (`base_speed`, từ 15% đến 100%, bước nhảy $\pm 5\%$). Hệ thống tự động liên kết $Max = Speed + 35\%$ giúp xe tự bứt tốc trên đường thẳng.
2. **`2. Kp:`** Hệ số tỉ lệ bẻ lái vào cua (bước nhảy $\pm 0.005$).
3. **`3. Kd:`** Hệ số vi phân chống lắc đuôi xe khi chạy thẳng (bước nhảy $\pm 0.020$).
4. **`4. Phanh:`** Mức độ phanh lùi bánh trong và phanh cứng góc vuông $90^\circ$ (từ **`0%`** đến **`80%`**, bước nhảy $\pm 5\%$).
   * *`0%`:* Tắt phanh lùi, bánh trong chỉ giảm tới 0%. Phù hợp sàn bám hoặc tránh nóng driver.
   * *`35% - 50%`:* Phanh lùi vừa phải (mặc định), xe ôm cua mượt mà.
   * *`60% - 80%`:* Phanh lùi gắt và xoay tại chỗ cực nhanh ở góc $90^\circ$.
5. **`5. Truon:`** Thời gian trườn tới đỉnh góc vuông trước khi phanh xoay (từ **`0ms`** đến **`200ms`**, bước nhảy $\pm 10\text{ms}$, mặc định **`70ms`**).
   * Do thanh cảm biến QTR-8A nằm nhô ra trước trục bánh xe một khoảng $5\text{cm} \dots 8\text{cm}$, khi cảm biến vừa thấy góc vuông thì bánh xe vẫn chưa tới góc.
   * Mục `Truon` cho phép xe tiếp tục tiến thẳng thêm một khoảng thời gian ngắn để đưa trục bánh xe tới đúng đỉnh góc vuông rồi mới phanh xoay, triệt tiêu hoàn toàn hiện tượng xe quay sớm trước góc làm chém vào trong và văng ra ngoài.
   * *Xe quay sớm trước góc:* Tăng thêm $+10\text{ms} \dots +20\text{ms}$ (ví dụ: lên 80ms - 90ms).
   * *Xe chạy lố quá góc:* Giảm bớt $-10\text{ms} \dots -20\text{ms}$ (ví dụ: xuống 50ms - 60ms).
6. **`6. Line:`** Chế độ vạch màu (`Den` trên nền trắng hoặc `Trang` trên nền đen).
7. **`7. Calib:`** Kích hoạt chế độ quét tay hiệu chuẩn cảm biến ngoài sân (xong tự động lưu Flash).
8. **`8. Luu:`** Lưu toàn bộ tham số vào bộ nhớ Flash nội (Page 63) vĩnh viễn.
9. **`9. Thoat:`** Lưu cấu hình và quay về màn hình chính.

---

## 3. Thao tác 4 nút bấm chuẩn xác

| Nút bấm | Chân MCU | Ở Màn hình Chính (`STOPPED`) | Trong Menu Cài đặt (`MENU`) | Khi đang chạy (`RUNNING`) |
| :---: | :---: | :--- | :--- | :--- |
| **Nút 1 (`BTN_RUN`)** | **`PB3`** | **CHẠY XE** | **LƯU FLASH & THOÁT (OK)** | **DỪNG KHẨN CẤP** |
| **Nút 2 (`BTN_MENU`)** | **`PB4`** | **MỞ MENU** | **GIẢM GIÁ TRỊ (`-`)** / Chọn | **DỪNG KHẨN CẤP** |
| **Nút 3 (`BTN_UP`)** | **`PB5`** | *(Không tác dụng)* | **TĂNG GIÁ TRỊ (`+`)** / Chọn | **DỪNG KHẨN CẤP** |
| **Nút 4 (`BTN_DOWN`)** | **`PB6`** | *(Không tác dụng)* | **DI CHUYỂN XUỐNG (`v`)** | **DỪNG KHẨN CẤP** |

> [!NOTE]
> Driver nút bấm đã được cấu hình nhận diện chuẩn xác mức logic LOW (`Bit_RESET`) khi nhấn nút nối GND, có bộ lọc rung 20ms nhạy bén và tính năng Auto-Repeat khi nhấn giữ nút `+` hoặc `-` trong Menu.
