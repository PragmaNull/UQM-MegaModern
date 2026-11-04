"""Build the project (cmake --build), reconfiguring first if needed. Optionally run clang-tidy."""

import argparse
import os
import re
import shutil
import sys
from pathlib import Path

from _common import PROJECT_ROOT, cached_config, env_script, fail, make_parser, run
from setup import configure

FIXES_DIR_NAME = "clang-tidy-fixes"

# Initializations that clang-tidy writes as ' = value'. The project uses braced initialization.
ZERO_INIT = re.compile(r"(ReplacementText:\s*)'\s*=\s*(?:0|0\.0|0\.0F|0U|nullptr|false)'")
VALUE_INIT = re.compile(r"(ReplacementText:\s*)'\s*=\s*([^']+)'")


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


def llvm_tool(name):
    """Path of an LLVM tool next to clang-tidy."""
    clang_tidy = shutil.which("clang-tidy")
    if clang_tidy is None:
        fail("clang-tidy is not on the PATH")
    tool = Path(clang_tidy).parent / name
    if not tool.is_file() and not tool.with_suffix(".exe").is_file():
        fail(f"{name} is not next to {clang_tidy}")
    return tool


def run_clang_tidy(build_dir, fixes_dir=None, jobs=None):
    """Run clang-tidy with the checks in .clang-tidy on the project sources in the compile database."""
    cmd = [
        sys.executable, llvm_tool("run-clang-tidy"),
        "-p", build_dir,
        "-quiet",
        "-source-filter", r".*[/\\](src|test)[/\\].*",
    ]
    if jobs:
        cmd += ["-j", str(jobs)]
    if fixes_dir is not None:
        cmd += ["-export-fixes", fixes_dir]
    run(cmd)


def apply_tidy_fixes(fixes_dir):
    """Apply the fixes that clang-tidy exported, with braced initialization and left const."""
    files = sorted(Path(fixes_dir).glob("*.yaml"))
    if not files:
        print("clang-tidy found nothing to fix.")
        return
    for file in files:
        text = file.read_text(encoding="utf-8")
        text = ZERO_INIT.sub(r"\1' {}'", text)
        text = VALUE_INIT.sub(r"\1' {\2}'", text)
        file.write_text(text, encoding="utf-8")
    # The changed lines get the format of src/.clang-format, which puts const on the left.
    run([llvm_tool("clang-apply-replacements"), "-format", "-style=file", f"-style-config={PROJECT_ROOT / 'src'}", fixes_dir])
    print(f"Applied the fixes from {len(files)} files.")


def main():
    parser = make_parser(
        "Build the project. Reconfigures if --config/--asan differ from the existing build.",
        config_default=None)
    parser.add_argument("-t", "--target", help="build only this target")
    parser.add_argument("-j", "--jobs", type=int, help="number of parallel jobs")
    parser.add_argument("--clean", action="store_true", help="clean before building")
    parser.add_argument("--clang-tidy", action="store_true",
                        help="after the build, run clang-tidy with the checks in .clang-tidy")
    parser.add_argument("--tidy-fix", action="store_true",
                        help="after the build, run clang-tidy and apply its fixes to the sources")
    args, extra = parser.parse_known_args()

    # The cmake options go before the arguments for the build tool, which follow '--'.
    options = []
    if args.target:
        options += ["--target", args.target]
    if args.jobs:
        options += ["--parallel", str(args.jobs)]
    if args.clean:
        options += ["--clean-first"]
    build(args.build_dir, args.config, args.asan, [*options, *extra])

    if args.clang_tidy or args.tidy_fix:
        fixes_dir = None
        if args.tidy_fix:
            fixes_dir = Path(args.build_dir)
            fixes_dir = (fixes_dir if fixes_dir.is_absolute() else PROJECT_ROOT / fixes_dir) / FIXES_DIR_NAME
            shutil.rmtree(fixes_dir, ignore_errors=True)
            fixes_dir.mkdir(parents=True)
        run_clang_tidy(args.build_dir, fixes_dir, args.jobs or os.cpu_count())
        if fixes_dir is not None:
            apply_tidy_fixes(fixes_dir)


if __name__ == "__main__":
    sys.exit(main())
