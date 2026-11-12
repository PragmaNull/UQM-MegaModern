#!/usr/bin/env bash
# Runs the input scripts in test/scripts against the game. See scripts/regress.py.
exec python3 "$(dirname "$0")/scripts/regress.py" "$@"
