#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
python3 "$ROOT/scripts/build.py" --verify
exec python3 -m unittest discover -s "$ROOT/tests" -p 'test_*.py' -v
