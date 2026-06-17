#!/usr/bin/env bash
# One command: install deps + run signal bridge (keep terminal open).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
exec python3 "$ROOT/tools/signal_bridge_run.py" "$@"
