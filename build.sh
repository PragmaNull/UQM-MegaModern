#!/usr/bin/env bash
# Builds the project. See scripts/build.py.
exec python3 "$(dirname "$0")/scripts/build.py" "$@"
