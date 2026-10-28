"""Run the unit tests (ctest), optionally building first."""

import sys

from _common import cached_config, env_script, fail, make_parser, run
from build import build


def main():
    parser = make_parser("Run the unit tests via ctest.", config_default=None)
    parser.add_argument("--build", action="store_true",
                        help="build the test target before running")
    parser.add_argument("-R", "--filter", help="only run tests matching this regex")
    parser.add_argument("-j", "--jobs", type=int, help="number of tests to run in parallel")
    args, extra = parser.parse_known_args()

    if args.build:
        config = build(args.build_dir, args.config, args.asan, ["--target", "UQM_Tests"])
    else:
        config, _ = cached_config(args.build_dir)
        if config is None:
            fail(f"{args.build_dir} is not configured; run 'setup' or pass --build")
        if args.config and args.config != config:
            fail(f"build dir is configured for {config}, not {args.config}; "
                 f"pass --build to reconfigure and rebuild")

    cmd = ["ctest", "--test-dir", args.build_dir, "--output-on-failure",
           "--build-config", config]
    if args.filter:
        cmd += ["-R", args.filter]
    if args.jobs:
        cmd += ["--parallel", str(args.jobs)]
    run(cmd + extra, env=env_script(args.build_dir, config, "conanrun"))


if __name__ == "__main__":
    sys.exit(main())
