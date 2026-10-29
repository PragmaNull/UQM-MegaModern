# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

UQM-MegaModern is a C++20 modernization of The Ur-Quan Masters (a port of Star Control II) combined with the HD-mod. Active effort is underway to replace legacy C patterns with modern C++ (STL abstractions, `<cstdint>` types, `enum class`, smart pointers).

## Build Instructions

### Helper scripts (Windows)

Root `.bat` files wrap Python scripts in `scripts/`:

```bat
setup.bat   [-c CONFIG] [--asan] [-p PROFILE]   :: conan install, then cmake configure (Ninja)
build.bat   [-c CONFIG] [--asan] [-t TARGET] [-j N] [--clean]
test.bat    [-c CONFIG] [--build] [-R REGEX] [-j N]
run.bat     [--build] [GAME ARGS...]           :: run the game with -n content
regress.bat [--build] [-R REGEX] [-j N]         :: run the input scripts in test/scripts
```

- `CONFIG`: `Release` (default), `Debug`, `RelWithDebInfo`, `MinSizeRel`. All take `-B DIR` (default `conanbuild`).
- `build`/`test` default to the config already in `CMakeCache.txt`; `build` reconfigures if `--config`/`--asan` differ.
- `setup --skip-install` runs only the cmake step. `build` runs conan install itself when the conan files for the config are missing.
- Commands run inside conan's `conanbuild`/`conanrun` env scripts, so no VS developer prompt is needed.
- Unrecognized arguments are forwarded to cmake/ctest. Scripts can also be run directly with `python scripts/<name>.py`.

### CMake + Conan (primary method)

```bash
# Install dependencies
pip install conan
conan install . -of conanbuild --build=missing

# Configure and build
cmake . -G Ninja -B conanbuild \
  -DCMAKE_TOOLCHAIN_FILE="conanbuild/build/Release/generators/conan_toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build conanbuild
```

### With AddressSanitizer

```bash
conan install . -of conanbuild --build=missing -o enable_asan=True --profile=asan
cmake . -G Ninja -B conanbuild \
  -DCMAKE_TOOLCHAIN_FILE="conanbuild/build/RelWithDebInfo/generators/conan_toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build conanbuild
```

## Running Tests

Tests use Google Test, discovered automatically via CMake.

```bash
cd conanbuild
ctest --output-on-failure --build-config Release

# Or run the test binary directly
./Release/test/UQM_Tests
```

Test sources are in `test/tests/` organized by subsystem (core, lib, options, uqm).

Input scripts in `test/scripts/` drive the game menus. `regress.bat` runs each one with a new config dir and fails on a non-zero exit code or an `[error]` or `[critical]` log line. A `<name>.settings.json` file next to a script is copied in as the start settings. `regress.bat` runs the game muted and in a window. `run.bat` does the same with `--input-script`, unless the volume or `-f` options are given.

## Code Architecture

### Entry Point

`src/main.cpp` → `UQMGame::setup()` → `UQMGame::run()` → `UQMGame::teardown()`

### Module Structure

| Directory | Purpose |
|-----------|---------|
| `src/core/` | Foundational utilities: logging (spdlog), STL abstraction layer, string utilities, platform-specific code |
| `src/libs/` | Engine subsystems: graphics (SDL2), sound (OpenAL/Vorbis), input, video, threading, Lua scripting, netplay |
| `src/uqm/` | Game logic: dialogue system, planet mechanics, ship definitions, combat (supermelee), galaxy/starmap |
| `src/options/` | CLI and config file parsing (CLI11) |
| `src/configuration/` | Build-time configuration headers |
| `test/` | GTest unit tests |
| `doc/devel/` | 40+ developer documentation files covering architecture, formats, and subsystems |

### Key Abstractions

**STL abstraction layer** (`src/core/stl/`): All code uses `uqstl::` namespace aliases instead of `std::` directly (e.g., `uqstl::vector`, `uqstl::shared_ptr`). This allows future substitution with EASTL or other implementations.

**Logging** (`src/core/log/`): Wraps spdlog with backtrace capture. Use this rather than `printf`/`cout`.

**Content**: Game data assets are a separate repository (`UQM-MegaModern-Content`) and must be placed in `content/`.

## Code Conventions

- **Namespaces**: `uqm::` for game code, `uqstl::` for container/algorithm aliases, `uqgsl::` for GSL-lite aliases
- **Classes**: PascalCase; member variables prefixed `m_`; static variables prefixed `s_`
- **C++ standard**: C++20 required
- **Integer types**: Use `<cstdint>` types (`int32_t`, `uint8_t`, etc.) — not legacy custom types
- **Enums**: Use `enum class` with `magic_enum` for reflection where needed
- **String safety**: Use `uqstl::string_view` for safe string views (pass by value); use `uqgsl::czstring` when the API deals with C-style null-terminated strings; use `scn::scan` instead of `sscanf`
- **Comments**: Keep comments minimal. Write them in Simplified Technical English. Do not explain "why", and do not refer to other classes or files unless necessary; such comments read as notes to self. Get user approval before adding a comment longer than 3 lines. Start every new source file with a one-line comment that states its purpose.
- **C++ Core Guidelines**: Follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) as closely as possible: RAII for all resources, no owning raw pointers, `const` by default, no C-style casts, prefer value types and the rule of zero
- **Initialization**: Use braced initialization throughout — for variables (`int x{42};`), and for class members/locals with default constructors (`uqstl::vector<int> v{};`) even when the default constructor would be called implicitly

## Formatting

Run clang-format on all modified files after making changes:

```bash
{ git diff --name-only --diff-filter=M; git ls-files --others --exclude-standard; } \
  | grep -E '\.(cpp|h|hpp|c)$' \
  | xargs clang-format -i
```

Format rules are in `src/.clang-format`.

## Commits

Do not add a `Co-Authored-By: Claude` line or any other Claude attribution to commit messages.
