"""Shared helpers for the install/setup/build/test scripts."""

import argparse
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = PROJECT_ROOT / "conanbuild"
CONFIGS = ("Release", "Debug", "RelWithDebInfo", "MinSizeRel")
IS_WINDOWS = os.name == "nt"


def make_parser(description, config_default="Release"):
    """Create an argument parser with the common options."""
    parser = argparse.ArgumentParser(
        description=description,
        epilog="Unrecognised arguments are passed through to the underlying tool.",
    )
    parser.add_argument(
        "-c", "--config",
        type=parse_config,
        default=config_default,
        help=f"build configuration: {', '.join(CONFIGS)} "
             f"(default: {config_default or 'value from the existing build'})",
    )
    parser.add_argument(
        "--asan",
        action=argparse.BooleanOptionalAction,
        default=None,
        help="enable AddressSanitizer",
    )
    parser.add_argument(
        "-B", "--build-dir",
        type=Path,
        default=DEFAULT_BUILD_DIR,
        help=f"build output directory (default: {DEFAULT_BUILD_DIR.name})",
    )
    return parser


def parse_config(value):
    for config in CONFIGS:
        if config.lower() == value.lower():
            return config
    raise argparse.ArgumentTypeError(
        f"invalid config '{value}' (choose from {', '.join(CONFIGS)})")


def generators_dir(build_dir, config):
    return Path(build_dir) / "build" / config / "generators"


def env_script(build_dir, config, name):
    """Path to a conan-generated environment script (conanbuild/conanrun), if present."""
    suffix = ".bat" if IS_WINDOWS else ".sh"
    script = generators_dir(build_dir, config) / f"{name}{suffix}"
    return script if script.is_file() else None


def read_cmake_cache(build_dir):
    """Return the CMakeCache.txt entries as a dict, or an empty dict if not configured."""
    cache_file = Path(build_dir) / "CMakeCache.txt"
    if not cache_file.is_file():
        return {}
    entries = {}
    pattern = re.compile(r"^([^#/][^:=]*):[^=]*=(.*)$")
    for line in cache_file.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            entries[match.group(1)] = match.group(2)
    return entries


def cached_config(build_dir):
    """Configuration and ASAN state of an already-configured build dir, or (None, None)."""
    cache = read_cmake_cache(build_dir)
    if not cache:
        return None, None
    asan = cache.get("ENABLE_ASAN", "OFF").upper() in ("ON", "TRUE", "1", "YES")
    return cache.get("CMAKE_BUILD_TYPE") or None, asan


def run(cmd, env=None):
    """Run a command, optionally inside a conan environment script, exiting on failure."""
    cmd = [str(part) for part in cmd]
    print(f"> {format_cmd(cmd)}", flush=True)
    if env is not None:
        if IS_WINDOWS:
            inner = f'call "{env}" >nul && {subprocess.list2cmdline(cmd)}'
            full_cmd = f'cmd /d /s /c "{inner}"'
        else:
            full_cmd = ["bash", "-c", f'. "{env}" && exec {shlex.join(cmd)}']
    else:
        full_cmd = cmd
    result = subprocess.run(full_cmd, cwd=PROJECT_ROOT, env=process_env())
    if result.returncode != 0:
        sys.exit(result.returncode)


def process_env():
    """Return the child process environment. On Windows, add vswhere.exe to PATH."""
    env = os.environ.copy()
    if IS_WINDOWS:
        program_files = env.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
        installer = Path(program_files) / "Microsoft Visual Studio" / "Installer"
        if (installer / "vswhere.exe").is_file():
            env["PATH"] = f"{env.get('PATH', '')}{os.pathsep}{installer}"
    return env


def format_cmd(cmd):
    return subprocess.list2cmdline(cmd) if IS_WINDOWS else shlex.join(cmd)


def fail(message):
    print(f"error: {message}", file=sys.stderr)
    sys.exit(1)
