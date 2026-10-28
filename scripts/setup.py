"""Generate the CMake build files using the conan toolchain."""

import sys
from pathlib import Path

from _common import (cached_config, env_script, fail, generators_dir, make_parser,
                     read_cmake_cache, run)


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
    toolchain = generators_dir(build_dir, config) / "conan_toolchain.cmake"
    if not toolchain.is_file():
        asan_flag = " --asan" if asan else ""
        fail(f"{toolchain} not found; run 'install --config {config}{asan_flag}' first")

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
    parser = make_parser("Generate CMake build files for the selected configuration.")
    args, extra = parser.parse_known_args()
    configure(args.build_dir, args.config, bool(args.asan), extra)


if __name__ == "__main__":
    sys.exit(main())
