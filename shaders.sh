#!/usr/bin/env bash
# Compiles the HLSL shaders to headers. See scripts/build_shaders.py.
exec python3 "$(dirname "$0")/scripts/build_shaders.py" "$@"
