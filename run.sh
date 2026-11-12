#!/usr/bin/env bash
# Runs the game with the content directory. See scripts/run.py.
exec python3 "$(dirname "$0")/scripts/run.py" "$@"
