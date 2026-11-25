# Cài bench HIL trên PC Linux

Dành cho Ubuntu/Debian 64-bit. Mọi lệnh chạy bằng user sẽ sở hữu bench (không phải root); script tự gọi `sudo` khi cần.

Các thiết lập riêng của máy (cổng serial, đường dẫn công cụ, thư mục dữ liệu) nằm trong **một file**:
`~/.config/hil-ci/hil-ci.env`. Code đọc chúng qua biến môi trường; không đặt thì dùng giá trị mặc định của Windows (`COM6`, `COM7`, `COM9`...), nên bench Windows cũ vẫn chạy như trước.

| Biến | Dùng cho |
|---|---|
| `HIL_PORT`, `HIL_DUT_PORT`, `HIL_LOG_PORT` | Cổng serial của board HIL, DUT, Pico 2 |
| `HIL_STM32_PROGRAMMER` | `STM32_Programmer_CLI` để nạp DUT |
| `HIL_CUBEIDE` | `stm32cubeide` để CI tự build repo không có hex |
| `HIL_CI_DATA` | Clone repo user, kết quả, trạng thái |
| `HIL_VENV` | Python venv có robotframework, pyserial, numpy |

## 1. Lấy code

Máy mới thường chưa có git (script ở bước 4 cũng cài git, nhưng phải có git mới lấy được script):

```bash
sudo apt update && sudo apt install -y git
git clone https://github.com/vohuynhhuyhung/Hardware_in_the_Loop-System.git ~/Hardware_in_the_Loop-System
cd ~/Hardware_in_the_Loop-System
```

## 2. Cài STM32CubeProgrammer (bắt buộc: nạp firmware cho DUT)

ST bắt đăng nhập để tải, nên tải trên máy có trình duyệt rồi chép sang:

1. Tải **STM32CubeProgrammer**, mục OS chọn **Generic Linux** cho máy x86_64 (`uname -m`), ví dụ
   `SetupSTM32CubeProgrammer_2.24.0_linux-x86_64.zip`.
2. Chép sang máy Linux, chạy trên Windows (PowerShell):
   `scp "$env:USERPROFILE\Downloads\SetupSTM32CubeProgrammer_*_linux-x86_64.zip" <user>@<host>:~/`
3. Cài ở chế độ console (không cần màn hình):
   ```bash
   sudo apt install -y unzip libusb-1.0-0
   cd ~ && unzip SetupSTM32CubeProgrammer_*_linux-x86_64.zip -d stm32cubeprg
   ./stm32cubeprg/SetupSTM32CubeProgrammer-*.linux -console
   ```
   Chấp nhận thư mục mặc định `~/STMicroelectronics/STM32Cube/STM32CubeProgrammer`.
4. Kiểm tra: `~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI --version`

## 3. Cài STM32CubeIDE (chỉ cần nếu có repo không commit sẵn hex)

1. Tải **STM32CubeIDE Debian Linux Installer** bản 1.19.0 (`st-stm32cubeide_1.19.0_*_amd64.deb_bundle.sh.zip`), chép sang như trên.
2. Cài:
   ```bash
   unzip st-stm32cubeide_1.19.0_*_amd64.deb_bundle.sh.zip
   sudo sh ./st-stm32cubeide_1.19.0_*_amd64.deb_bundle.sh
   ```
   Thư mục cài mặc định: `/opt/st/stm32cubeide_1.19.0/`. Cài bản khác thì sửa `HIL_CUBEIDE` ở bước 5.

## 4. Chạy script cài đặt

```bash
bash HIL_ci/linux/setup_linux.sh
```

Script làm 6 việc: cài gói hệ thống, tạo venv `~/hil-venv`, cho user quyền dùng cổng serial và USB
(nhóm `dialout`, `plugdev`, udev rule `99-hil-bench.rules`), tạo `~/.config/hil-ci/hil-ci.env`, cài service
`hil-watcher` (chưa bật), và kiểm tra các công cụ ở bước 2, 3.

**Sau đó đăng xuất rồi đăng nhập lại** (ssh lại) để quyền nhóm `dialout` có hiệu lực.

## 5. Cắm bench và điền cổng serial

Cắm USB của board HIL, Pico 2 và DUT vào máy Linux. Cổng của DUT chỉ xuất hiện khi DUT có nguồn,
mà nguồn DUT do board HIL bật. Board HIL và DUT cùng USB ID `0483:5740`, chỉ khác số serial.

```bash
bash HIL_ci/linux/find_ports.sh                      # thấy board HIL (0483) và Pico (2e8a)
~/hil-venv/bin/python HIL_ci/hil_power.py on --port /dev/serial/by-id/<cổng board HIL>
bash HIL_ci/linux/find_ports.sh                      # cổng 0483 mới xuất hiện là DUT
~/hil-venv/bin/python HIL_ci/hil_power.py off --port /dev/serial/by-id/<cổng board HIL>
```

Không biết cổng 0483 nào là board HIL: thử `hil_power.py status --port ...` với từng cổng; cổng trả lời là board HIL.

Điền ba cổng vào `~/.config/hil-ci/hil-ci.env`, luôn dùng tên trong `/dev/serial/by-id/`
(`/dev/ttyACM0, 1, 2` đổi thứ tự mỗi lần DUT khởi động lại):

```
HIL_PORT=/dev/serial/by-id/usb-STMicroelectronics_STM32_Virtual_ComPort_<serial HIL>-if00
HIL_DUT_PORT=/dev/serial/by-id/usb-STMicroelectronics_STM32_Virtual_ComPort_<serial DUT>-if00
HIL_LOG_PORT=/dev/serial/by-id/usb-Raspberry_Pi_Pico_<serial>-if00
```

## 6. Chạy thử bằng tay

```bash
set -a; . ~/.config/hil-ci/hil-ci.env; set +a
# một suite trên firmware đang có trong DUT
$HIL_VENV/bin/python -m robot --pythonpath HIL_robot/libraries --variable HIL_ROOT:$PWD \
    --outputdir /tmp/hil_try HIL_robot/tests/01__check_connection.robot
# cả pipeline: clone, lấy hex, nạp qua DFU, chạy hil_tests/ của repo DUT
$HIL_VENV/bin/python HIL_ci/poll_registry.py --only dut --ref main
```

Kết quả ở `~/hil_ci_data/results/dut/<sha>/`. Lần nạp đầu tiên là phép thử quan trọng nhất: nó kiểm tra
cùng lúc quyền udev cho DFU, việc phát hiện DFU trên Linux và `STM32_Programmer_CLI`.

## 7. Bật watcher

```bash
sudo systemctl enable --now hil-watcher.service    # chạy ngay và tự chạy lại sau mỗi lần khởi động máy
journalctl -u hil-watcher -f                       # xem log trực tiếp
tail -f ~/hil_ci_data/watcher.log
```

Sau khi đổi `hil-ci.env` hoặc `git pull` repo HIL: `sudo systemctl restart hil-watcher`.

Watcher không cần màn hình hay đăng nhập: nó chạy từ lúc máy khởi động, kể cả khi không ai ssh vào.

## 8. (Tùy chọn) GitHub Actions runner

Chỉ cần nếu muốn chạy CI tay từ tab Actions. Vào repo HIL → Settings → Actions → Runners → New self-hosted runner
→ Linux, làm theo hướng dẫn, thêm nhãn `hil`, rồi cài thành service: `sudo ./svc.sh install && sudo ./svc.sh start`.
Workflow tự đọc `~/.config/hil-ci/hil-ci.env` của user chạy runner. Gỡ runner trên máy Windows cũ để job không chạy nhầm sang đó.

## Lỗi thường gặp

| Triệu chứng | Cách sửa |
|---|---|
| `Permission denied: '/dev/ttyACM0'` | Chưa đăng nhập lại sau bước 4 (`groups` phải có `dialout`) |
| `STM32 DFU device not detected` | `lsusb` khi DUT đang ở bootloader phải thấy `0483:df11`; kiểm tra cáp USB của DUT |
| Nạp lỗi `No DFU capable USB device` | udev rule chưa áp dụng: rút/cắm lại DUT hoặc `sudo udevadm trigger` |
| Lệnh đầu tiên tới DUT/HIL bị rác | ModemManager đang dò cổng: `sudo systemctl disable --now ModemManager` |
| `STM32CubeIDE not found` | Sửa `HIL_CUBEIDE` cho đúng thư mục cài, hoặc dùng hex dựng sẵn |
| Headless build báo lỗi display/GTK | `sudo apt install xvfb`, rồi chạy watcher qua `xvfb-run` (sửa dòng `ExecStart` của service) |
