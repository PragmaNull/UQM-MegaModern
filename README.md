# UQM-MegaModern

UQM-MegaModern is a C++23 modernization of [UQM-MegaMod](https://github.com/JHGuitarFreak/UQM-MegaMod), which is a fork of [The Ur-Quan Masters](https://sc2.sourceforge.net/) (the open-source port of Star Control II) and the HD-mod.

The goal is to keep the game and the MegaMod features, and to replace the legacy C code with modern C++:

* STL containers and algorithms
* RAII and smart pointers in place of manual memory management
* All content and save files converted to JSON
* SDL3 graphics with GPU shaders, OpenAL sound, and Asio netplay
* Thread safety annotations that Clang checks
* Google Test unit tests, save round-trip tests, and scripted input regression tests
* Replace hand-crafted libraries with well established 3rd party libraries where possible.

This project is a work in progress. Save files, content files, and settings are not compatible with UQM, MegaMod, or other mods.

Report problems at the [issue tracker](https://github.com/PragmaNull/UQM-MegaModern/issues).

## Changes from UQM-MegaMod

This project forked from UQM-MegaMod at commit `18e1f3d1` (2026-01-29). The numbers below compare that commit with this repository in November 2026. The 518 MegaMod commits after the fork are not in this project.

### Size

| | MegaMod at the fork | MegaModern |
|---|---:|---:|
| Commits after the fork | — | 380 |
| C source files in `src` | 413 | 0 |
| C++ source files in `src` | 1 | 333 |
| Lines of C/C++ code in `src` | 243,839 | 201,433 |
| Lines without bundled third-party code and generated shader headers | 208,374 | 188,398 (−10%) |
| Unit tests (Google Test) | 0 | 367 |
| CTest tests | 0 | 593 |

Line counts do not include blank lines. Across the full repository, the diff from the fork is 1,935 files changed, +263,739 and −402,589 lines.

### Code that was removed or replaced

* **Bundled third-party code (35,465 lines):** Lua, MikMod, regex, getopt, and MD5 come from Conan packages.
* **Old UQM libraries:** `libs/uio` (12,930 lines) is replaced by `libs/io` on `std::filesystem`. `libs/threads`, `heap`, `list`, `task`, `memory`, and `log` are replaced by the standard library and spdlog. `libs/cdp` and `libs/decomp` are removed.
* **Platforms and build systems:** the Darwin and Symbian code, and the `build.sh`, Makefile, and MSVC project builds, are replaced by CMake and Conan.
* **Libraries:** SDL2 is replaced by SDL3, with GPU shaders for the scalers. Netplay uses Asio.

### Legacy patterns

Number of lines in `src` that contain the pattern, without bundled third-party code and generated headers:

| Pattern | At the fork | Now |
|---|---:|---:|
| Legacy integer types (`BYTE`, `COUNT`, `SIZE`, `BOOLEAN`, ...) | 5,951 | 0 |
| `NULL` | 3,182 | 0 |
| `HMalloc` / `HFree` | 408 | 0 |
| `printf` / `fprintf` | 480 | 10 |
| `sprintf` / `strcpy` / `strcat` / `strncpy` | 310 | 5 |
| `sscanf` | 36 | 0 |
| `malloc` / `free` | 73 | 20 |
| `volatile` | 42 | 1 |
| `goto` | 195 | 78 |
| `#define` | 5,091 | 537 |
| `constexpr` | 0 | 3,028 |
| `enum class` | 0 | 44 |
| `UQM_GUARDED_BY` / `UQM_REQUIRES` | 0 | 70 |

### Bug fixes in the MegaMod code

The conversion found and fixed 13 bugs in the code from MegaMod: 8 memory leaks, 2 data races, 2 problems with HD content at SD scale, and a hash that sign-extended bytes. [FIXES.md](FIXES.md) lists them with their MegaMod locations and the commits that fix them here.

### New features

* A seed menu for each new game, and the seed of the game in the Game Options menu.
* ImGui windows that show and change the game state, the settings, and the cheats.
* `--check-save`, `--play-save`, and `--input-script` for automated tests.

## Content

The game data is in a separate repository: [UQM-MegaModern-Content](https://github.com/PragmaNull/UQM-MegaModern-Content).

> [!IMPORTANT]
> Install the content before you run the game. Download or clone the content repository and copy *all* of its files into the `content` folder of this repository. The `content` folder must then contain `base`, `addons`, and the `*.key.json` files.

## Building

### Requirements

* Windows 10 or later, or Linux. macOS builds are not supported yet.
* [CMake](https://cmake.org/) and [Ninja](https://ninja-build.org/)
* [Python](https://www.python.org/) 3 and [Conan](https://conan.io/) 2 (`pip install conan`)

Conan installs the libraries that the game uses. See [conanfile.py](conanfile.py) for the list.

#### Windows

Make a Conan profile for `clang-cl` with `conan profile detect`, then set the compiler in the profile. For example:

```ini
[settings]
arch=x86_64
build_type=Release
compiler=clang
compiler.cppstd=20
compiler.runtime=dynamic
compiler.runtime_type=Release
compiler.version=21
compiler.runtime_version=v145
os=Windows

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.build:compiler_executables={"c": "clang-cl.exe", "cpp": "clang-cl.exe"}
```

#### Linux

The Linux build uses GCC 15 or later. These steps are for Debian and Ubuntu, and work in WSL2.

1. Install the build tools. Install Conan with `pipx`, because `pip install` fails with `externally-managed-environment`:

   ```bash
   sudo apt-get install -y build-essential cmake ninja-build python3 pipx pkgconf
   pipx install conan
   conan profile detect
   ```

2. Install the system development packages for SDL (X11, OpenGL, EGL and udev). Conan does not install them, and `setup.sh` stops with an error if one is missing:

   ```bash
   sudo apt-get install -y libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev \
     libxau-dev libxaw7-dev libxcomposite-dev libxcursor-dev libxdamage-dev libxdmcp-dev \
     libxext-dev libxfixes-dev libxi-dev libxinerama-dev libxkbfile-dev libxmu-dev \
     libxmuu-dev libxpm-dev libxrandr-dev libxrender-dev libxres-dev libxss-dev libxt-dev \
     libxtst-dev libxv-dev libxxf86vm-dev libxcb-glx0-dev libxcb-render0-dev \
     libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev libxcb-image0-dev \
     libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev \
     libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-dri3-dev uuid-dev libxcb-cursor-dev \
     libxcb-dri2-0-dev libxcb-present-dev libxcb-composite0-dev libxcb-ewmh-dev \
     libxcb-res0-dev libxcb-util-dev libgl-dev libudev-dev libegl-dev
   ```

3. Use the `.sh` scripts in place of the `.bat` scripts below. They take the same arguments. The default build directory on Linux is `conanbuild-linux`, so that a checkout shared with Windows (for example in WSL) can hold both builds.

The first `setup.sh` builds many libraries from source and can take a long time.

### Build and run

From the repository root:

```bat
setup.bat
build.bat
run.bat
```

On Linux:

```bash
./setup.sh
./build.sh
./run.sh
```

* `setup.bat [-c CONFIG] [--asan] [-p PROFILE]` installs the libraries and configures CMake.
* `build.bat [-c CONFIG] [-t TARGET] [--clean]` builds the game and the tests.
* `run.bat [--build] [GAME ARGS...]` runs the game with the content in `content`.

`CONFIG` is `Release` (default), `Debug`, `RelWithDebInfo`, or `MinSizeRel`.

To build without the scripts:

```bash
conan install . -of conanbuild --build=missing
cmake . -G Ninja -B conanbuild \
  -DCMAKE_TOOLCHAIN_FILE="conanbuild/build/Release/generators/conan_toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build conanbuild
```

### Tests

```bat
test.bat      :: unit tests and save tests (ctest)
regress.bat   :: input scripts in test/scripts
```

On Linux, use `./test.sh` and `./regress.sh`. `test --build` builds the game and the tests first, because the save tests run the game.

See [CLAUDE.md](CLAUDE.md) for more information about the build options, the tests, and the code conventions. The developer documents are in [doc/devel](doc/devel).

## Features

The MegaMod features and setup options are described in [doc/release/MegaMod-README.txt](doc/release/MegaMod-README.txt). The changes to MegaMod up to the fork are in [doc/release/CHANGELOG.txt](doc/release/CHANGELOG.txt).

> [!WARNING]
> **Controllers:** do not use DS4Windows with a DualShock 4 controller. The game supports the DualShock 4 without extra software, and the controller layout options do not work correctly with DS4Windows.

## Credits

* Fred Ford and Paul Reiche III, for the original Star Control II
* The Ur-Quan Masters team, for the open-source port
* The HD-mod team
* The MegaMod team: JHGuitarFreak, SlightlyIntelligentMonkey, Volasaurus, Ala-lala, Kruzenshtern, Taleden, and Jordan Lo

The main menu music is by Saibuster (Itamar Levy), Mark Vera (Jouni Airaksinen), and Rush AX. The default Super Melee menu music is by Flashira Nakirov.

The full list of authors is in [AUTHORS](AUTHORS).

## License

The code is under the GNU General Public License, version 2. See [LICENSE](LICENSE). The content has its own license; see the content repository.
