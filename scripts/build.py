"""Build the project (cmake --build), reconfiguring first if needed."""

import sys

from _common import cached_config, env_script, make_parser, run
from setup import configure


def build(build_dir, config, asan, extra=()):
    """Build, (re)running the CMake configure step if the build dir doesn't match."""
    current_config, current_asan = cached_config(build_dir)
    config = config or current_config or "Release"
    asan = current_asan if asan is None else asan
    if (config, bool(asan)) != (current_config, bool(current_asan)):
        configure(build_dir, config, bool(asan))

    run(["cmake", "--build", build_dir, *extra],
        env=env_script(build_dir, config, "conanbuild"))
    return config


def main():
    parser = make_parser(
        "Build the project. Reconfigures if --config/--asan differ from the existing build.",
        config_default=None)
    parser.add_argument("-t", "--target", help="build only this target")
    parser.add_argument("-j", "--jobs", type=int, help="number of parallel jobs")
    parser.add_argument("--clean", action="store_true", help="clean before building")
    args, extra = parser.parse_known_args()

    if args.target:
        extra += ["--target", args.target]
    if args.jobs:
        extra += ["--parallel", str(args.jobs)]
    if args.clean:
        extra += ["--clean-first"]
    build(args.build_dir, args.config, args.asan, extra)


if __name__ == "__main__":
    sys.exit(main())
