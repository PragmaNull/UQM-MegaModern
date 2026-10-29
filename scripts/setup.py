"""Install the conan dependencies and generate the CMake build files."""

import json
import subprocess
import sys
from pathlib import Path

from _common import (cached_config, env_script, fail, generators_dir, make_parser,
                     read_cmake_cache, run)


def profile_settings(profile):
    """Host settings of a conan profile, or an empty dict if they can't be read."""
    try:
        result = subprocess.run(
            ["conan", "profile", "show", "-pr", profile, "--format=json"],
            capture_output=True, text=True, check=True)
        return json.loads(result.stdout)["host"]["settings"]
    except (OSError, subprocess.CalledProcessError, ValueError, KeyError):
        return {}


def install(build_dir, config, asan, profile="default", policy="missing"):
    cmd = [
        "conan", "install", ".",
        "-of", build_dir,
        f"--build={policy}",
        "-pr:a", profile,
        "-s", f"build_type={config}",
        "-o", f"&:enable_asan={asan}",
    ]

    # Set the C runtime type to match the build type.
    settings = profile_settings(profile)
    if not settings:
        fail(f"could not read conan profile '{profile}' "
             f"(create one with 'conan profile detect')")
    if "compiler.runtime_type" in settings:
        runtime_type = "Debug" if config == "Debug" else "Release"
        cmd += ["-s", f"compiler.runtime_type={runtime_type}"]

    run(cmd)


def cache_is_stale(build_dir, config, asan):
    """Return True if the CMake cache is for a different config or other conan generators."""
    current_config, current_asan = cached_config(build_dir)
    if current_config is None:
        return False
    if (current_config, current_asan) != (config, asan):
        return True
    expected = generators_dir(build_dir, config).resolve()
    for key, value in read_cmake_cache(build_dir).items():
        if key.endswith("_DIR") and Path(value).name == "generators":
            if Path(value).resolve() != expected:
                return True
    return False


def configure(build_dir, config, asan, extra=()):
    """Run the CMake configure step. Installs the conan dependencies first if they are missing."""
    toolchain = generators_dir(build_dir, config) / "conan_toolchain.cmake"
    if not toolchain.is_file():
        install(build_dir, config, asan)

    cmd = [
        "cmake", ".",
        "-G", "Ninja",
        "-B", build_dir,
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DCMAKE_BUILD_TYPE={config}",
        f"-DENABLE_ASAN={'ON' if asan else 'OFF'}",
        *extra,
    ]
    if cache_is_stale(build_dir, config, asan):
        cmd.append("--fresh")
    run(cmd, env=env_script(build_dir, config, "conanbuild"))


def main():
    parser = make_parser("Install the conan dependencies and generate CMake build files "
                         "for the selected configuration.")
    parser.add_argument(
        "-p", "--profile",
        default="default",
        help="conan profile to use for host and build (default: default)",
    )
    parser.add_argument(
        "--build-policy",
        default="missing",
        help="conan --build policy (default: missing)",
    )
    parser.add_argument("--skip-install", action="store_true",
                        help="do not run conan install if the dependencies are already installed")
    args, extra = parser.parse_known_args()

    asan = bool(args.asan)
    if not args.skip_install:
        install(args.build_dir, args.config, asan, args.profile, args.build_policy)
    configure(args.build_dir, args.config, asan, extra)


if __name__ == "__main__":
    sys.exit(main())
