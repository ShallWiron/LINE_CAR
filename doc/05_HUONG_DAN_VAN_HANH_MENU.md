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

## 2. Giao diện Menu Cài đặt tập trung (`MENU`)

Toàn bộ các tham số cấu hình của xe được dồn gọn vào Menu gồm 7 mục:

```
+--------------------------------+
|      === MENU CAI DAT ===      |
| > 1.Toc do:  45%               |
|   2.Kp:      0.040             |
|   3.Kd:      0.350             |
|   4.Line:    Den               |
|   B4:v  B3:+  B2:-  B1:OK      |
+--------------------------------+
```

### Danh sách 7 mục cài đặt trong Menu:
1. **`1. Toc do:`** Tốc độ cơ sở (`base_speed`, từ 15% đến 100%, bước nhảy $\pm 5\%$). Hệ thống tự động liên kết $Max = Speed + 35\%$ giúp xe tự bứt tốc trên đường thẳng.
2. **`2. Kp:`** Hệ số tỉ lệ bẻ lái vào cua (bước nhảy $\pm 0.005$).
3. **`3. Kd:`** Hệ số vi phân chống lắc đuôi xe khi chạy thẳng (bước nhảy $\pm 0.020$).
4. **`4. Line:`** Chế độ vạch màu (`Den` trên nền trắng hoặc `Trang` trên nền đen).
5. **`5. Calib:`** Kích hoạt chế độ quét tay hiệu chuẩn cảm biến ngoài sân (xong tự động lưu Flash).
6. **`6. Luu:`** Lưu toàn bộ tham số vào bộ nhớ Flash nội (Page 63) vĩnh viễn.
7. **`7. Thoat:`** Lưu cấu hình và quay về màn hình chính.

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
