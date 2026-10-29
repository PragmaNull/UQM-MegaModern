"""Run the input scripts in test/scripts against the game, in parallel."""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from _common import DEFAULT_BUILD_DIR, IS_WINDOWS, PROJECT_ROOT, fail
from build import build

CONTENT_DIR = PROJECT_ROOT / "content"
SCRIPTS_DIR = PROJECT_ROOT / "test" / "scripts"
ERROR_LINE = re.compile(r"\[(error|critical)\]")


def run_script(exe, script, out_dir, timeout):
    """Run one input script with a new config dir. Returns (name, ok, message)."""
    name = script.stem
    config_dir = out_dir / name
    shutil.rmtree(config_dir, ignore_errors=True)
    config_dir.mkdir(parents=True)
    # A <name>.settings.json file next to the script is the start settings.
    settings = script.with_suffix(".settings.json")
    if settings.is_file():
        shutil.copy(settings, config_dir / "settings.json")
    log = out_dir / f"{name}.log"

    cmd = [exe, "-n", CONTENT_DIR, "-C", config_dir, "-l", log,
           "--input-script", script, "--fullscreen", "Windowed",
           "--musicvol", "0", "--sfxvol", "0", "--speechvol", "0"]
    try:
        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=timeout)
    except subprocess.TimeoutExpired:
        return name, False, f"timed out after {timeout} s"
    if result.returncode != 0:
        return name, False, f"exit code {result.returncode}"
    if not log.is_file():
        return name, False, "no log"
    errors = [line for line in log.read_text(encoding="utf-8", errors="replace").splitlines() if ERROR_LINE.search(line)]
    if errors:
        return name, False, f"{len(errors)} error lines, first: {errors[0].strip()}"
    return name, True, ""


def main():
    parser = argparse.ArgumentParser(description="Run the input scripts in test/scripts against the game.")
    parser.add_argument(
        "-B", "--build-dir",
        type=Path,
        default=DEFAULT_BUILD_DIR,
        help=f"build output directory (default: {DEFAULT_BUILD_DIR.name})",
    )
    parser.add_argument("--build", action="store_true", help="build the game before running")
    parser.add_argument("-R", "--filter", help="only run scripts whose name matches this regex")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count(),
                        help="number of scripts to run in parallel (default: number of CPUs)")
    parser.add_argument("-o", "--out-dir", type=Path,
                        help="directory for the config dirs and logs (default: a new temporary directory)")
    parser.add_argument("--timeout", type=int, default=300, help="seconds before a script fails (default: 300)")
    args = parser.parse_args()

    if args.build:
        build(args.build_dir, None, None, ["--target", "UrQuanMasters"])

    exe = Path(args.build_dir) / ("UrQuanMasters.exe" if IS_WINDOWS else "UrQuanMasters")
    if not exe.is_file():
        fail(f"{exe} not found; run 'build' first or pass --build")
    if not CONTENT_DIR.is_dir():
        fail(f"{CONTENT_DIR} not found; clone UQM-MegaModern-Content there")

    scripts = sorted(SCRIPTS_DIR.glob("*.txt"))
    if args.filter:
        scripts = [script for script in scripts if re.search(args.filter, script.stem)]
    if not scripts:
        fail("no input scripts match")

    out_dir = args.out_dir or Path(tempfile.mkdtemp(prefix="uqm-regress-"))
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"Running {len(scripts)} scripts, {args.jobs} at a time. Logs: {out_dir}", flush=True)

    failed = 0
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = [pool.submit(run_script, exe, script, out_dir, args.timeout) for script in scripts]
        for future in futures:
            name, ok, message = future.result()
            print(f"{'PASS' if ok else 'FAIL'}  {name}{'  ' + message if message else ''}", flush=True)
            failed += 0 if ok else 1

    print(f"{len(scripts) - failed}/{len(scripts)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
