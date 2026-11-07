"""Run the full HIL pipeline for ONE user repo at ONE ref.

    pull -> get .hex (prebuilt or build) -> smoke + flash -> user's hil_tests/ -> power off

The caller must hold hil_lock() (poll_registry.py does). Running this file
directly takes the lock itself.

Optional manifest `hil.json` in the user repo root:
    {
      "tests_dir": "hil_tests",     # default "hil_tests"
      "hex": "build/fw.hex",        # prebuilt hex: used as-is, no build
      "build_dir": "Debug"          # dir with the CubeIDE makefile (default: Debug/)
    }
"""
import argparse
import json
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

from hil_ci_common import (BUILD_TIMEOUT_S, CI_DIR, HIL_ROOT, MAKE_CMD,
                           PREPARE_TIMEOUT_S, RESULTS_DIR, USER_TEST_TIMEOUT_S,
                           WORK_DIR, hil_lock, log)

LIBS_DIR = HIL_ROOT / "HIL_robot" / "libraries"


class PipelineError(Exception):
    def __init__(self, stage, msg):
        super().__init__(msg)
        self.stage = stage


def _git(*args, cwd):
    return subprocess.run(["git", *args], cwd=cwd, check=True,
                          capture_output=True, text=True, timeout=300).stdout.strip()


def checkout(name, repo, ref):
    work = WORK_DIR / name
    work.mkdir(parents=True, exist_ok=True)
    try:
        if not (work / ".git").exists():
            _git("init", cwd=work)
            _git("remote", "add", "origin", repo, cwd=work)
        _git("remote", "set-url", "origin", repo, cwd=work)
        _git("fetch", "--depth", "1", "origin", ref, cwd=work)
        _git("checkout", "--force", "--detach", "FETCH_HEAD", cwd=work)
        _git("clean", "-ffdx", cwd=work)
        return work, _git("rev-parse", "HEAD", cwd=work)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        detail = getattr(e, "stderr", "") or str(e)
        raise PipelineError("pull", f"git failed: {detail.strip()}")


def _inside(work, rel):
    """Resolve a manifest path and refuse anything that escapes the checkout."""
    p = (work / rel).resolve()
    if not p.is_relative_to(work.resolve()):
        raise PipelineError("manifest", f"Path escapes the repo: {rel}")
    return p


def get_hex(work, manifest):
    """Prebuilt hex from the manifest if declared, otherwise build it."""
    if manifest.get("hex"):
        hex_path = _inside(work, manifest["hex"])
        if not hex_path.is_file():
            raise PipelineError("build", f"hil.json declares hex '{manifest['hex']}' but it is not in the repo")
        log(f"Using prebuilt hex: {hex_path}")
        return hex_path

    if manifest.get("build_dir"):
        build_dir = _inside(work, manifest["build_dir"])
    elif (work / "Debug" / "makefile").exists():
        build_dir = work / "Debug"
    else:
        found = sorted(work.glob("**/Debug/makefile"))
        if not found:
            raise PipelineError("build", "No hex declared and no Debug/makefile found to build from")
        build_dir = found[0].parent
    if not (build_dir / "makefile").exists():
        raise PipelineError("build", f"No makefile in {build_dir}")
    if shutil.which(MAKE_CMD) is None:
        raise PipelineError("build", f"'{MAKE_CMD}' not found on the runner (set HIL_MAKE, e.g. CubeIDE's make.exe)")

    log(f"Building in {build_dir}")
    try:
        r = subprocess.run([MAKE_CMD, "-C", str(build_dir), "all", "-j4"],
                           capture_output=True, text=True, timeout=BUILD_TIMEOUT_S)
    except subprocess.TimeoutExpired:
        raise PipelineError("build", f"Build timed out after {BUILD_TIMEOUT_S}s")
    if r.returncode != 0:
        raise PipelineError("build", "Build failed:\n" + (r.stdout + r.stderr)[-3000:])
    hexes = sorted(build_dir.glob("*.hex"), key=lambda p: p.stat().st_mtime)
    if not hexes:
        raise PipelineError("build", "Build succeeded but produced no .hex")
    return hexes[-1]


def check_hex(hex_path):
    with open(hex_path, "r", errors="replace") as f:
        first = f.readline().strip()
    if hex_path.stat().st_size == 0 or not first.startswith(":"):
        raise PipelineError("build", f"{hex_path.name} is not a valid Intel HEX file")


def robot(stage, suite, outdir, timeout, extra_vars):
    """Run robot; cwd=outdir so hil_logs/ and logic-analyzer .bin land with the results."""
    outdir.mkdir(parents=True, exist_ok=True)
    cmd = [sys.executable, "-m", "robot",
           "--outputdir", str(outdir),
           "--pythonpath", str(LIBS_DIR),  # one deterministic cli_lib for all libraries
           "--variable", f"HIL_ROOT:{HIL_ROOT.as_posix()}"]
    for k, v in extra_vars.items():
        cmd += ["--variable", f"{k}:{v}"]
    if stage == "prepare":
        cmd.append("--exitonfailure")
    cmd.append(str(suite))
    log(f"[{stage}] {' '.join(cmd)}")
    try:
        return subprocess.run(cmd, cwd=outdir, timeout=timeout).returncode
    except subprocess.TimeoutExpired:
        log(f"[{stage}] timed out after {timeout}s")
        return 124


def run(name, repo, ref):
    name = re.sub(r"[^A-Za-z0-9_.-]", "_", name)
    started = time.time()
    summary = {"name": name, "repo": repo, "ref": ref, "sha": None,
               "status": "ERROR", "stage": None, "message": "", "stages": {}}
    results_dir = None
    ran_robot = False
    try:
        work, sha = checkout(name, repo, ref)
        summary["sha"] = sha
        results_dir = RESULTS_DIR / name / sha[:12]
        if results_dir.exists():
            shutil.rmtree(results_dir)
        results_dir.mkdir(parents=True)
        log(f"{name} @ {sha[:12]}")

        manifest_file = work / "hil.json"
        manifest = json.loads(manifest_file.read_text("utf-8")) if manifest_file.exists() else {}

        hex_path = get_hex(work, manifest)
        check_hex(hex_path)
        summary["stages"]["build"] = "PASS"

        tests_dir = _inside(work, manifest.get("tests_dir", "hil_tests"))
        if not tests_dir.is_dir():
            raise PipelineError("tests", f"Tests dir not found: {tests_dir.name}/")

        vars_ = {"FIRMWARE_PATH": hex_path.as_posix()}

        ran_robot = True
        rc = robot("prepare", CI_DIR / "robot" / "prepare.robot",
                   results_dir / "prepare", PREPARE_TIMEOUT_S, vars_)
        summary["stages"]["prepare"] = "PASS" if rc == 0 else "FAIL"
        if rc != 0:
            raise PipelineError("prepare", "Bench smoke check or flash failed (see prepare/log.html)")

        rc = robot("user", tests_dir, results_dir / "user", USER_TEST_TIMEOUT_S, vars_)
        summary["stages"]["user_tests"] = "PASS" if rc == 0 else "FAIL"
        summary["status"] = "PASS" if rc == 0 else "FAIL"
        summary["stage"] = "user_tests"
        summary["message"] = "" if rc == 0 else (
            "Test run timed out" if rc == 124 else f"{rc} test(s) failed" if rc < 250 else f"robot exit code {rc}")
    except PipelineError as e:
        summary["status"] = "ERROR"
        summary["stage"] = e.stage
        summary["message"] = str(e)
        log(f"ERROR in {e.stage}: {e}")
    except Exception as e:  # keep the bench safe whatever happens
        summary["message"] = f"{type(e).__name__}: {e}"
        log(f"ERROR: {summary['message']}")
    finally:
        if ran_robot and results_dir:
            robot("cleanup", CI_DIR / "robot" / "cleanup.robot",
                  results_dir / "cleanup", 120, {})
        summary["duration_s"] = round(time.time() - started, 1)
        if results_dir:
            (results_dir / "summary.json").write_text(json.dumps(summary, indent=2), "utf-8")
    log(f"RESULT {name}: {summary['status']} {summary['message']}")
    summary["results_dir"] = str(results_dir) if results_dir else None
    return summary


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--name", required=True)
    ap.add_argument("--repo", required=True, help="git URL of the user repo")
    ap.add_argument("--ref", default="main", help="branch or commit SHA")
    a = ap.parse_args()
    with hil_lock():
        res = run(a.name, a.repo, a.ref)
    sys.exit(0 if res["status"] == "PASS" else 1)
