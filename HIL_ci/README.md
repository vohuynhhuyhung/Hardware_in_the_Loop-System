# HIL CI

Pipeline chạy trên PC nối với bench HIL (Windows hoặc Linux).

```
GitHub Actions (cron 5 phút / chạy tay)
  └─ poll_registry.py      đọc registry.json, so SHA mới nhất với last_tested.json
       └─ run_hil_tests.py  (mỗi commit mới, giữ khóa bench)
            1. git fetch + checkout commit của user
            2. hex: dùng "hex" trong hil.json nếu có, không thì build bằng STM32CubeIDE headless
            3. robot/prepare.robot : smoke HIL + flash        (của nền tảng)
            4. robot <hil_tests/>  : test của user
            5. robot/cleanup.robot : tắt nguồn DUT (luôn chạy)
```

## Watcher (kích hoạt nhanh, không cần token)
`HIL_ci/watch_registry.py` chạy nền trên máy bench, mỗi 30 giây hỏi `git ls-remote` cho từng repo trong `registry.json`
và chạy pipeline ngay khi có commit mới. Không cần GitHub Actions, runner hay secret ở repo user.
```
python HIL_ci/watch_registry.py              # chạy mãi
python HIL_ci/watch_registry.py --once       # một lượt rồi thoát (để thử)
```
- Log: `D:\HCMUT\DA2\hil_ci_data\watcher.log`; `watcher.heartbeat` ghi thời điểm lượt kiểm tra gần nhất.
- `registry.json` được đọc lại mỗi lượt, thêm repo không cần khởi động lại.
- Dùng chung state và khóa bench với `poll_registry.py` nên chạy song song với cron không bị trùng.
- Chạy từ bản clone của repo HIL trên máy bench; nhớ `git pull` khi nền tảng có thay đổi.

## Đăng ký repo user
Thêm vào `registry.json` (qua PR, bạn duyệt = cổng tin cậy):
```json
{ "repos": [ { "name": "nhom1", "repo": "https://github.com/nhom1/fw.git", "branches": ["main"] } ] }
```

## Repo của user
- Project STM32CubeIDE (commit `.project`, `.cproject`, mã nguồn; CI tự build headless) hoặc hex dựng sẵn.
- `hil_tests/*.robot` (xem `user_template/`), import keyword HIL bằng
  `Resource ${HIL_ROOT}/HIL_robot/resources/HIL_keywords.robot`.
- `hil.json` (tùy chọn): `tests_dir`, `hex`, `project`, `config` (xem docstring `run_hil_tests.py`).
- `.gitignore` của repo này đang chặn `*.hex`; user muốn dùng hex dựng sẵn phải `git add -f`.

## Cài runner (một lần)
1. Cài Python, STM32CubeIDE (build headless bằng `stm32cubeidec.exe`, đổi đường dẫn bằng biến `HIL_CUBEIDE`), STM32CubeProgrammer.
2. Đăng ký GitHub self-hosted runner với label `hil`.
3. Dữ liệu (clone, kết quả, trạng thái) nằm ở `D:\HCMUT\DA2\hil_ci_data` (đổi bằng biến `HIL_CI_DATA`).

## Chạy tay
```
python HIL_ci/run_hil_tests.py --name nhom1 --repo <git-url> --ref main
python HIL_ci/poll_registry.py --only nhom1 --ref <branch-hoac-sha>
```

## Thiết lập riêng của máy bench
Đọc từ biến môi trường; không đặt thì dùng giá trị mặc định của bench Windows.

| Biến | Mặc định (Windows) | Dùng ở |
|---|---|---|
| `HIL_PORT` | `COM6` | `HIL_robot/resources/HIL_keywords.robot`, `hil_power.py` |
| `HIL_LOG_PORT` | `COM9` | `HIL_robot/resources/HIL_keywords.robot`, `Logic_analyzer/tools/*.py` |
| `HIL_DUT_PORT` | `COM7` | `HIL_robot/user_resources/DUT_keywords.robot` |
| `HIL_STM32_PROGRAMMER` | PATH, rồi `D:\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe` | `HIL_robot/libraries/hil_lib.py` |
| `HIL_CUBEIDE` | `D:\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeidec.exe` | `HIL_ci/hil_ci_common.py` |
| `HIL_CI_DATA` | `D:\HCMUT\DA2\hil_ci_data` (Linux: `~/hil_ci_data`) | `HIL_ci/hil_ci_common.py` |

Timeout nằm trong `HIL_ci/hil_ci_common.py`.

## Bench Linux
Xem [linux/README.md](linux/README.md): script `linux/setup_linux.sh` cài gói, venv, quyền USB/serial (udev),
file thiết lập `~/.config/hil-ci/hil-ci.env` và service systemd `hil-watcher`.

Bật/tắt nguồn DUT bằng tay (mọi hệ điều hành): `python HIL_ci/hil_power.py on|off|status`.
