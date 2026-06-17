"""Transport mode resolution for signal bridge (ws / uart / ble)."""

from __future__ import annotations

import os
from typing import Any, Literal

TransportName = Literal["auto", "ble", "uart", "ws"]
VALID_TRANSPORTS: tuple[str, ...] = ("auto", "ble", "uart", "ws")


def resolve_transport(requested: TransportName, cfg: dict[str, Any]) -> TransportName:
    """Honor CLI flag, then SIGNAL_BRIDGE_TRANSPORT env, then config file."""
    if requested != "auto":
        return requested

    env = os.environ.get("SIGNAL_BRIDGE_TRANSPORT", "").strip().lower()
    if env in VALID_TRANSPORTS and env != "auto":
        return env  # type: ignore[return-value]

    mode = str(cfg.get("transport", "ws")).strip().lower()
    if mode in VALID_TRANSPORTS:
        return mode  # type: ignore[return-value]
    return "ws"
