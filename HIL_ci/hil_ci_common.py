"""Shared paths, settings and the HIL bench lock for the HIL CI scripts."""
import contextlib
import os
import sys
import time
from pathlib import Path

CI_DIR = Path(__file__).resolve().parent
HIL_ROOT = CI_DIR.parent  # root of the HIL platform repo
IS_WINDOWS = sys.platform == "win32"

# ---------------------------------------------------------------------------
# Bench settings. Every one can be overridden with an environment variable
# (on Linux they live in the env file read by the watcher service, see HIL_ci/linux/).
# The other bench settings (serial ports, STM32_Programmer_CLI) are read from the
# same variables by HIL_robot/resources/*.robot and HIL_robot/libraries/hil_lib.py.
# ---------------------------------------------------------------------------
# Persistent data (clones of user repos, last-tested state, results).
# It MUST live outside the GitHub Actions workspace: actions/checkout runs
# `git clean -ffdx` and would wipe it on every run.
DATA_DIR = Path(os.environ.get("HIL_CI_DATA") or
                (r"D:\HCMUT\DA2\hil_ci_data" if IS_WINDOWS else str(Path.home() / "hil_ci_data")))
WORK_DIR = DATA_DIR / "work"
RESULTS_DIR = DATA_DIR / "results"
STATE_FILE = DATA_DIR / "last_tested.json"
LOCK_FILE = DATA_DIR / "hil.lock"

# STM32CubeIDE launcher used for the headless build of user repos that do not commit a hex
# (Windows: the console build stm32cubeidec.exe; Linux: the default install dir of the ST installer).
CUBEIDE_CMD = os.environ.get("HIL_CUBEIDE") or (
    r"D:\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeidec.exe" if IS_WINDOWS
    else "/opt/st/stm32cubeide_1.19.0/stm32cubeide")
BUILD_TIMEOUT_S = 600
PREPARE_TIMEOUT_S = 300   # smoke + flash
USER_TEST_TIMEOUT_S = 1800
LOCK_TIMEOUT_S = 3600     # how long a job waits for the bench


def log(msg):
    print(f"[hil-ci] {msg}", flush=True)


def _try_lock(f):
    if sys.platform == "win32":
        import msvcrt
        f.seek(0)
        msvcrt.locking(f.fileno(), msvcrt.LK_NBLCK, 1)
    else:
        import fcntl
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)


def _unlock(f):
    if sys.platform == "win32":
        import msvcrt
        f.seek(0)
        msvcrt.locking(f.fileno(), msvcrt.LK_UNLCK, 1)
    else:
        import fcntl
        fcntl.flock(f, fcntl.LOCK_UN)


@contextlib.contextmanager
def hil_lock(timeout=LOCK_TIMEOUT_S):
    """Exclusive lock on the physical bench. The OS drops it if the process dies."""
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    f = open(LOCK_FILE, "a+")
    locked = False
    deadline = time.time() + timeout
    try:
        while True:
            try:
                _try_lock(f)
                locked = True
                break
            except OSError:
                if time.time() > deadline:
                    raise TimeoutError("Timed out waiting for the HIL bench lock")
                time.sleep(2)
        yield
    finally:
        if locked:
            _unlock(f)
        f.close()
