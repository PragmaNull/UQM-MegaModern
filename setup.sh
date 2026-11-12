#!/usr/bin/env bash
# Installs the conan dependencies and generates the CMake build files. See scripts/setup.py.
exec python3 "$(dirname "$0")/scripts/setup.py" "$@"
