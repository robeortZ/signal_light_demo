#!/usr/bin/env python3
"""
Cursor hook handler: map agent lifecycle events to signal light status.

Reads hook JSON from stdin; phase name from SIGNAL_HOOK_PHASE env (set by shell wrapper).
Logs to tools/logs/signal_bridge.log with timestamps (same file as cursor_signal_bridge.py).
"""

from __future__ import annotations

import json
import logging
import os
import sys
from typing import Any

from cursor_signal_bridge import push_status
from signal_bridge_log import log_path, new_event_id, setup_bridge_logging

try:
    from cursor_signal_keepalive import ensure_running, set_keepalive_status
except ImportError:
    ensure_running = None  # type: ignore[assignment,misc]
    set_keepalive_status = None  # type: ignore[assignment,misc]

log = logging.getLogger("signal_hook")

VALID = {"idle", "working", "attention", "urgent"}


def _parse_input() -> dict[str, Any]:
    raw = os.environ.get("SIGNAL_HOOK_INPUT", "").strip()
    if not raw:
        return {}
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        return {}


def _hook_summary(data: dict[str, Any]) -> str:
    tool = data.get("tool_name") or data.get("toolName") or ""
    cmd = data.get("command") or data.get("shellCommand") or ""
    parts = []
    if tool:
        parts.append(f"tool={tool}")
    if cmd:
        text = str(cmd).replace("\n", " ")[:120]
        parts.append(f"cmd={text!r}")
    code = data.get("exitCode", data.get("exit_code"))
    if code is not None:
        parts.append(f"exit={code}")
    return " ".join(parts)


def _map_phase(phase: str, data: dict[str, Any]) -> str | None:
    tool = str(data.get("tool_name") or data.get("toolName") or "")
    tool_lower = tool.lower()

    if phase in ("start", "tool", "shell", "thought"):
        if "askquestion" in tool_lower or "userquestion" in tool_lower:
            return "attention"
        return "working"

    if phase == "fail":
        return "urgent"

    if phase == "after_shell":
        code = data.get("exitCode", data.get("exit_code", 0))
        try:
            if int(code) != 0:
                return "urgent"
        except (TypeError, ValueError):
            pass
        return "working"

    if phase == "stop":
        return "idle"

    if phase == "end":
        return "idle"

    if phase == "permission":
        return "attention"

    return None


def main() -> int:
    setup_bridge_logging("signal_hook", verbose=os.environ.get("SIGNAL_BRIDGE_VERBOSE") == "1")
    phase = os.environ.get("SIGNAL_HOOK_PHASE", "unknown")
    data = _parse_input()
    summary = _hook_summary(data)
    status = _map_phase(phase, data)
    hook_id = new_event_id()

    if status is None or status not in VALID:
        log.debug(
            "event=%s | action=hook_skip | phase=%s | %s",
            hook_id,
            phase,
            summary or "no payload",
        )
        return 0

    log.info(
        "event=%s | action=hook | phase=%s | map=%s | %s",
        hook_id,
        phase,
        status,
        summary or "no payload",
    )

    try:
        if status != "idle" and set_keepalive_status is not None and ensure_running is not None:
            set_keepalive_status(status)
            ensure_running()
        push_id = push_status(status, source=f"hook:{phase}", event_id=hook_id)
        log.info("event=%s | action=hook_ok | push_event=%s | log_file=%s", hook_id, push_id, log_path())
    except Exception as exc:  # noqa: BLE001 — hooks must not block agent
        log.error("event=%s | action=hook_fail | error=%s", hook_id, exc)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
