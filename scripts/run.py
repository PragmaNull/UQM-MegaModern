"""Run the game with the project's content directory."""

import argparse
import sys
from pathlib import Path

from _common import DEFAULT_BUILD_DIR, IS_WINDOWS, PROJECT_ROOT, fail, run
from build import build

CONTENT_DIR = PROJECT_ROOT / "content"


def main():
    parser = argparse.ArgumentParser(
        description="Run the game with the project's content directory.",
        epilog="Unrecognised arguments are passed through to the game.",
    )
    parser.add_argument(
        "-B", "--build-dir",
        type=Path,
        default=DEFAULT_BUILD_DIR,
        help=f"build output directory (default: {DEFAULT_BUILD_DIR.name})",
    )
    parser.add_argument("--build", action="store_true", help="build the game before running")
    args, extra = parser.parse_known_args()

    if args.build:
        build(args.build_dir, None, None, ["--target", "UrQuanMasters"])

    exe = Path(args.build_dir) / ("UrQuanMasters.exe" if IS_WINDOWS else "UrQuanMasters")
    if not exe.is_file():
        fail(f"{exe} not found; run 'build' first or pass --build")

    has_content_arg = any(arg in ("-n", "--contentdir") or arg.startswith("--contentdir=") for arg in extra)
    if not has_content_arg:
        if not CONTENT_DIR.is_dir():
            fail(f"{CONTENT_DIR} not found; clone UQM-MegaModern-Content there")
        extra = ["-n", CONTENT_DIR, *extra]

    # Run scripted games muted and in a window.
    def has_arg(*names):
        return any(str(arg) in names or str(arg).startswith(tuple(f"{name}=" for name in names)) for arg in extra)

    if has_arg("--input-script"):
        if not has_arg("-f", "--fullscreen"):
            extra = [*extra, "--fullscreen", "Windowed"]
        for short, long in (("-M", "--musicvol"), ("-S", "--sfxvol"), ("-T", "--speechvol")):
            if not has_arg(short, long):
                extra = [*extra, long, "0"]

    run([exe, *extra])


if __name__ == "__main__":
    sys.exit(main())
