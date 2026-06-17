#!/usr/bin/env bash
# Cursor hook: push agent status to T5 signal light (non-blocking).
# Bridge UART is owned by tools/run_bridge.sh — do not start a second daemon here.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PYTHON="$ROOT/tools/.venv/bin/python3"
if [[ ! -x "$PYTHON" ]]; then
  PYTHON="python3"
fi
export PYTHONPATH="$ROOT/tools${PYTHONPATH:+:$PYTHONPATH}"
export SIGNAL_HOOK_PHASE="${1:-unknown}"
export SIGNAL_HOOK_INPUT
SIGNAL_HOOK_INPUT="$(cat)"

"$PYTHON" "$ROOT/tools/cursor_signal_hook.py" </dev/null &
exit 0
