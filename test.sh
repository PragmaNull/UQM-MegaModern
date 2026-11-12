#!/usr/bin/env bash
# Runs the unit tests. See scripts/test.py.
exec python3 "$(dirname "$0")/scripts/test.py" "$@"
