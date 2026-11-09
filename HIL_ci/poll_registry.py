"""Poll every repo in registry.json; run the HIL pipeline for each new commit.

Only repos listed in registry.json are ever fetched or run. Adding a repo
there (via a reviewed PR to this repo) is the trust gate.
"""
import argparse
import json
import shutil
import subprocess
import sys

from hil_ci_common import CI_DIR, HIL_ROOT, STATE_FILE, hil_lock, log
from run_hil_tests import run


def remote_sha(repo, branch):
    try:
        out = subprocess.run(["git", "ls-remote", repo, f"refs/heads/{branch}"],
                             capture_output=True, text=True, timeout=60, check=True).stdout
        return out.split()[0] if out.strip() else None
    except Exception as e:
        log(f"ls-remote failed for {repo} ({branch}): {e}")
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", help="registry name to force-run, ignoring last-tested state")
    ap.add_argument("--ref", help="branch or SHA to use with --only")
    ap.add_argument("--artifacts-dir", default=str(HIL_ROOT / "hil_artifacts"),
                    help="copy results of this run here (for upload-artifact)")
    a = ap.parse_args()

    entries = json.loads((CI_DIR / "registry.json").read_text("utf-8")).get("repos", [])
    any_fail = False
    ran = 0

    with hil_lock():
        # Read the state only AFTER taking the bench lock: another process (watch_registry.py)
        # may have just tested this commit while we were waiting for the bench.
        state = json.loads(STATE_FILE.read_text("utf-8")) if STATE_FILE.exists() else {}
        for e in entries:
            if a.only and e["name"] != a.only:
                continue
            for branch in e.get("branches", ["main"]):
                if a.only and a.ref:
                    target = a.ref          # manual run at an explicit branch/SHA
                else:
                    target = remote_sha(e["repo"], branch)
                    if not target:
                        continue
                    if not a.only and state.get(e["name"], {}).get(branch) == target:
                        continue            # already tested this commit
                res = run(e["name"], e["repo"], target)
                ran += 1
                any_fail |= res["status"] != "PASS"
                if not (a.only and a.ref):
                    state.setdefault(e["name"], {})[branch] = target
                    STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
                    STATE_FILE.write_text(json.dumps(state, indent=2), "utf-8")
                if res.get("results_dir"):
                    dest = f"{a.artifacts_dir}/{e['name']}/{res['sha'][:12]}"
                    shutil.copytree(res["results_dir"], dest, dirs_exist_ok=True)

    log(f"Done: {ran} run(s), {'some FAILED' if any_fail else 'all OK'}")
    sys.exit(1 if any_fail else 0)


if __name__ == "__main__":
    main()
