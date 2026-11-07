# HIL CI – Ghi chú thiết kế (tóm tắt cuộc trao đổi)

Tài liệu này tóm tắt các quyết định và kiến thức đã thống nhất, để mang sang thư mục/phiên làm việc khác mà vẫn đủ ngữ cảnh.

## 1. Bối cảnh hệ thống

- **HIL**: STM32F407 + Raspberry Pi Pico 2 (giả lập logic analyzer). **DUT**: STM32F407 (hiện chỉ hỗ trợ loại F4).
- Giữa HIL và DUT: UART, SPI, I2C, GPIO, CAN, DAC/ADC, điều khiển nguồn DUT, chân BOOT0 của DUT.
- PC nối với HIL (COM6), Pico (COM9), DUT (COM7) qua serial; cổng USB của DUT cắm thẳng vào PC để flash.
- Test viết bằng **Robot Framework**; Robot chạy trên PC, gửi lệnh CLI qua serial tới HIL/DUT. Board HIL chỉ nhận lệnh và thao tác phần cứng, logic pass/fail nằm ở Robot/Python trên PC.

## 2. Phân chia trách nhiệm

| Phần | Ai viết | Vị trí |
|---|---|---|
| Thư viện + keyword HIL (power, flash, GPIO, analog, logic analyzer…) | Chủ nền tảng | `HIL_robot/libraries/`, `HIL_robot/resources/HIL_keywords.robot` |
| Keyword/thư viện điều khiển DUT theo CLI firmware của họ | User | `user_lib/dut_lib.py`, `user_resources/DUT_keywords.robot` |
| Test case của user | User | `hil_tests/*.robot` trong **repo của user** |
| Pipeline CI | Chủ nền tảng | `HIL_ci/`, `.github/workflows/hil-ci.yml` |

`HIL_robot/tests/01…13` là test hồi quy của chính nền tảng HIL, không chạy cho user.

## 3. Cách flash DUT (đã có sẵn)

Không dùng SPI. Dùng **USB DFU** của bootloader ROM STM32F4, chạy trên PC (`HIL_robot/libraries/hil_lib.py`, keyword `HIL Flash Firmware for DUT`):

1. HIL tắt nguồn DUT → kéo BOOT0 = 1 → bật nguồn DUT (DUT vào DFU).
2. PC dò thiết bị DFU (`wmic`), gọi `STM32_Programmer_CLI.exe -c port=USB1 -w <hex> 0x08000000 -v`.
3. HIL kéo BOOT0 = 0 → power cycle DUT → chạy firmware mới.

Keyword chỉ nhận **đường dẫn .hex có sẵn**, không build.

## 4. Kiến trúc pipeline đã chọn

```
GitHub Actions (self-hosted runner Windows, label "hil"; cron 5 phút hoặc chạy tay)
  └─ HIL_ci/poll_registry.py   đọc registry.json, so SHA branch với last_tested.json
       └─ HIL_ci/run_hil_tests.py   (mỗi commit mới, giữ khóa bench)
            1. git fetch + checkout commit của repo user
            2. hex: dùng "hex" trong hil.json nếu có; không thì make -C Debug
            3. robot/prepare.robot : smoke HIL + flash          (nền tảng sở hữu)
            4. robot <hil_tests/>  : test của user
            5. robot/cleanup.robot : tắt nguồn DUT (luôn chạy)
```

Quyết định chính:
- **Hex**: ưu tiên hex dựng sẵn do user khai trong `hil.json`; không có thì CI build bằng `Debug/makefile` do CubeIDE sinh (cần `arm-none-eabi-gcc` + `make`).
- **Pipeline thuộc repo HIL**, repo user chỉ chứa firmware + `hil_tests/` (+ `hil.json` tùy chọn). User không sửa được quy trình. Đăng ký repo bằng PR vào `HIL_ci/registry.json` = cổng tin cậy.
- **Flash là bước của runner**, không phải test của user (tránh user quên flash / test chạy trên firmware cũ).
- **Nhiều người dùng, một bench**: khóa file (`msvcrt`/`fcntl`) + `concurrency: hil-bench` của GitHub; trạng thái commit đã test riêng cho từng repo/branch.
- **Trạng thái/kết quả/clone** lưu ở `%USERPROFILE%\.hil_ci` (đổi bằng biến `HIL_CI_DATA`), **ngoài workspace**, vì `actions/checkout` chạy `git clean -ffdx` sẽ xóa sạch.
- Test của user nhận biến `HIL_ROOT` và `FIRMWARE_PATH`; import keyword bằng
  `Resource ${HIL_ROOT}/HIL_robot/resources/HIL_keywords.robot`.
- Robot chạy với `cwd` = thư mục kết quả của lần chạy, nên `hil_logs/` và file `.bin` của logic analyzer nằm cùng kết quả.

`hil.json` (tùy chọn, ở gốc repo user):
```json
{ "tests_dir": "hil_tests", "hex": "path/fw.hex", "build_dir": "Debug" }
```

## 5. Các file đã tạo

- `HIL_ci/hil_ci_common.py` – đường dẫn, hằng số, khóa bench
- `HIL_ci/run_hil_tests.py` – chạy pipeline cho một repo/ref
- `HIL_ci/poll_registry.py` – polling registry, chạy commit mới, copy kết quả ra `hil_artifacts/`
- `HIL_ci/robot/prepare.robot`, `HIL_ci/robot/cleanup.robot`
- `HIL_ci/registry.json`, `HIL_ci/requirements.txt`, `HIL_ci/README.md`
- `HIL_ci/user_template/` (`hil.json`, `hil_tests/example_hil_only.robot`)
- `.github/workflows/hil-ci.yml`

Lệnh chạy tay:
```
python HIL_ci/run_hil_tests.py --name nhom1 --repo <git-url> --ref main
python HIL_ci/poll_registry.py --only nhom1 --ref <branch-hoac-sha>
```

## 6. Trạng thái kiểm thử

- Đã chạy thử với repo git giả cục bộ: pull, đọc `hil.json`, dùng hex dựng sẵn, chạy stage prepare (fail ở COM6 vì máy không có HIL), cleanup vẫn chạy, `summary.json` được ghi. Cú pháp các script đã biên dịch OK.
- **Chưa kiểm thử**: flash thật, test của user trên phần cứng, workflow GitHub Actions, đường build bằng `make`.

## 7. Việc còn lại / lưu ý

1. Máy hiện tại **chưa có `make`** (chỉ có `arm-none-eabi-gcc`). Cài `make` hoặc đặt `HIL_MAKE` = `make.exe` của CubeIDE.
2. `.gitignore` đang chặn `*.hex`; user dùng hex dựng sẵn phải `git add -f`.
3. Hai `cli_lib.py` trùng tên nhưng khác nội dung (`libraries/` và `user_lib/`); runner ép bản của nền tảng (tập cha) đứng trước bằng `--pythonpath`. Về lâu dài nên đổi tên bản của user.
4. Test và `make` của user là code tùy ý chạy trên máy bạn → chỉ đăng ký repo tin cậy; luôn có timeout và bước tắt nguồn DUT.
5. Chưa có báo kết quả về commit/PR của user (cần token + quyền ghi repo user); hiện chỉ có job xanh/đỏ và artifact `log.html`.
6. Cron của GitHub có thể trễ; workflow bị tắt tự động nếu repo public không hoạt động 60 ngày.
7. `wmic` đã bị gỡ ở Windows 11 mới → nên đổi sang `Get-PnpDevice` hoặc `STM32_Programmer_CLI -l usb`.
8. Cài runner: Python, `arm-none-eabi-gcc`, `make`, STM32CubeProgrammer; đăng ký runner label `hil`.

## 8. Hardcode cần sửa khi đổi máy (đang giữ nguyên theo yêu cầu)

- `HIL_robot/resources/HIL_keywords.robot`: `COM6`, `COM9`
- `HIL_robot/user_resources/DUT_keywords.robot`: `COM7`
- `HIL_robot/libraries/hil_lib.py`: đường dẫn `STM32_Programmer_CLI.exe`, lệnh `wmic`
- `HIL_robot/tests/03__flash_fw_test.robot`: `FIRMWARE_PATH` (`D:/STUDY/...`)
- `HIL_ci/hil_ci_common.py`: `MAKE_CMD`, các timeout
