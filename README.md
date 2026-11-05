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

## Content

The game data is in a separate repository: [UQM-MegaModern-Content](https://github.com/PragmaNull/UQM-MegaModern-Content).

> [!IMPORTANT]
> Install the content before you run the game. Download or clone the content repository and copy *all* of its files into the `content` folder of this repository. The `content` folder must then contain `base`, `addons`, and the `*.key.json` files.

## Building

### Requirements

* Windows 10 or later. Linux and macOS builds are not supported yet.
* [CMake](https://cmake.org/) and [Ninja](https://ninja-build.org/)
* [Python](https://www.python.org/) 3 and [Conan](https://conan.io/) 2 (`pip install conan`)

Conan installs the libraries that the game uses. See [conanfile.py](conanfile.py) for the list.

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

### Build and run

From the repository root:

```bat
setup.bat
build.bat
run.bat
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
