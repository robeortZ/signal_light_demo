"""Unix-socket IPC for the shared UART daemon (hooks → daemon → serial)."""

from __future__ import annotations

import logging
import re
import socket
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
SOCK_PATH = TOOLS_DIR / "logs" / "signal_uart.sock"

log = logging.getLogger("uart_ipc")

SIGNAL_LINE_RE = re.compile(rb"^SIGNAL\s+(\w+)\s*\n?$")


def sock_path() -> Path:
    return SOCK_PATH


def daemon_available() -> bool:
    if SOCK_PATH.is_socket():
        return True
    return _tcp_ping()


def _tcp_ping() -> bool:
    try:
        with socket.create_connection(("127.0.0.1", 19876), timeout=0.3):
            return True
    except OSError:
        return False


def send_to_daemon(payload: bytes, *, timeout: float = 2.0) -> bool:
    if SOCK_PATH.is_socket():
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                client.settimeout(timeout)
                client.connect(str(SOCK_PATH))
                client.sendall(payload)
            log.debug("daemon ipc sent %d bytes (unix)", len(payload))
            return True
        except OSError as exc:
            log.debug("daemon ipc failed: %s", exc)
    try:
        with socket.create_connection(("127.0.0.1", 19876), timeout=timeout) as client:
            client.sendall(payload)
        log.debug("daemon ipc sent %d bytes (tcp)", len(payload))
        return True
    except OSError as exc:
        log.debug("daemon ipc failed: %s", exc)
        return False


def parse_signal_status(payload: bytes) -> str | None:
    match = SIGNAL_LINE_RE.match(payload.strip())
    if not match:
        return None
    return match.group(1).decode("ascii", errors="ignore")
