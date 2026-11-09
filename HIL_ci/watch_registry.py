"""Watch registry.json and run the HIL pipeline as soon as a registered repo gets a new commit.

Runs forever on the bench PC. Needs no GitHub Actions, no runner and no token:
it only uses `git ls-remote` (public repos) and the existing pipeline.

    python HIL_ci/watch_registry.py                # check every 30 s
    python HIL_ci/watch_registry.py --interval 20
    python HIL_ci/watch_registry.py --once         # one pass, then exit (for testing)

registry.json is re-read on every pass, so adding a repo needs no restart.
Results/state use the same folders as poll_registry.py, so both can coexist:
the bench lock serialises them and the state file prevents testing a SHA twice.
"""
import argparse
import json
import sys
import time
from datetime import datetime

from hil_ci_common import CI_DIR, DATA_DIR, STATE_FILE, hil_lock
from poll_registry import remote_sha
from run_hil_tests import run

LOG_FILE = DATA_DIR / "watcher.log"
HEARTBEAT_FILE = DATA_DIR / "watcher.heartbeat"


def log(msg):
    line = f"{datetime.now():%Y-%m-%d %H:%M:%S} [watcher] {msg}"
    print(line, flush=True)
    try:
        DATA_DIR.mkdir(parents=True, exist_ok=True)
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except OSError:
        pass


def load_state():
    try:
        return json.loads(STATE_FILE.read_text("utf-8")) if STATE_FILE.exists() else {}
    except (OSError, ValueError):
        return {}


def save_state(name, branch, sha):
    state = load_state()
    state.setdefault(name, {})[branch] = sha
    STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
    STATE_FILE.write_text(json.dumps(state, indent=2), "utf-8")


def load_registry():
    try:
        return json.loads((CI_DIR / "registry.json").read_text("utf-8")).get("repos", [])
    except (OSError, ValueError) as e:
        log(f"Cannot read registry.json: {e}")
        return []


def pass_once():
    """Check every registered repo/branch once. Returns the number of pipeline runs."""
    ran = 0
    for e in load_registry():
        name, repo = e.get("name"), e.get("repo")
        if not name or not repo:
            continue
        for branch in e.get("branches", ["main"]):
            sha = remote_sha(repo, branch)
            if not sha or load_state().get(name, {}).get(branch) == sha:
                continue
            log(f"New commit {name}/{branch} @ {sha[:12]}")
            try:
                with hil_lock():
                    # The cron job may have tested this SHA while we waited for the bench.
                    if load_state().get(name, {}).get(branch) == sha:
                        log(f"{name}/{branch} @ {sha[:12]} already tested, skipping")
                        continue
                    res = run(name, repo, sha)
                    save_state(name, branch, sha)  # also on FAIL/ERROR: never loop on a bad commit
                ran += 1
                log(f"{name}/{branch} @ {sha[:12]} -> {res['status']} {res.get('message', '')}")
            except TimeoutError as ex:
                log(f"Bench busy, will retry next pass: {ex}")
            except Exception as ex:  # keep the watcher alive whatever happens
                log(f"Unexpected error on {name}/{branch}: {type(ex).__name__}: {ex}")
    return ran


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--interval", type=int, default=30, help="seconds between passes (default 30)")
    ap.add_argument("--once", action="store_true", help="do a single pass and exit")
    a = ap.parse_args()

    log(f"Started. interval={a.interval}s, data={DATA_DIR}")
    while True:
        try:
            pass_once()
            DATA_DIR.mkdir(parents=True, exist_ok=True)
            HEARTBEAT_FILE.write_text(datetime.now().isoformat(), "utf-8")
        except KeyboardInterrupt:
            raise
        except Exception as ex:
            log(f"Pass failed: {type(ex).__name__}: {ex}")
        if a.once:
            break
        time.sleep(a.interval)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        log("Stopped.")
        sys.exit(0)
