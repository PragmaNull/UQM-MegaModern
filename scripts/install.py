"""Install conan dependencies (conan install)."""

import json
import subprocess
import sys

from _common import fail, make_parser, run


def profile_settings(profile):
    """Host settings of a conan profile, or an empty dict if they can't be read."""
    try:
        result = subprocess.run(
            ["conan", "profile", "show", "-pr", profile, "--format=json"],
            capture_output=True, text=True, check=True)
        return json.loads(result.stdout)["host"]["settings"]
    except (OSError, subprocess.CalledProcessError, ValueError, KeyError):
        return {}


def main():
    parser = make_parser("Install conan dependencies for the selected configuration.")
    parser.add_argument(
        "-p", "--profile",
        default="default",
        help="conan profile to use for host and build (default: default)",
    )
    parser.add_argument(
        "--build",
        default="missing",
        help="conan --build policy (default: missing)",
    )
    args, extra = parser.parse_known_args()

    cmd = [
        "conan", "install", ".",
        "-of", args.build_dir,
        f"--build={args.build}",
        "-pr:a", args.profile,
        "-s", f"build_type={args.config}",
        "-o", f"&:enable_asan={bool(args.asan)}",
    ]

    # Set the C runtime type to match the build type.
    settings = profile_settings(args.profile)
    if not settings:
        fail(f"could not read conan profile '{args.profile}' "
             f"(create one with 'conan profile detect')")
    if "compiler.runtime_type" in settings:
        runtime_type = "Debug" if args.config == "Debug" else "Release"
        cmd += ["-s", f"compiler.runtime_type={runtime_type}"]

    run(cmd + extra)


if __name__ == "__main__":
    sys.exit(main())
