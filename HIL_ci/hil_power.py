"""Switch the DUT supply by hand through the HIL board (Windows or Linux).

    python HIL_ci/hil_power.py on        # the DUT USB serial port appears a few seconds later
    python HIL_ci/hil_power.py off
    python HIL_ci/hil_power.py status

The HIL board port comes from --port, else the HIL_PORT environment variable, else COM6.
Leave the DUT off when done: the CI expects to find it powered off.
"""
import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "HIL_robot" / "libraries"))
from cli_lib import CLI  # noqa: E402

COMMANDS = {"on": "dut_power 1", "off": "dut_power 0", "status": "dut_power_status"}

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
ap.add_argument("action", choices=COMMANDS)
ap.add_argument("--port", default=os.environ.get("HIL_PORT", "COM6"))
ap.add_argument("--baud", type=int, default=921600)
a = ap.parse_args()

cli = CLI()
cli.connect(a.port, a.baud)
try:
    print(cli.execute(COMMANDS[a.action]))
finally:
    cli.disconnect()
