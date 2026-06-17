"""
Shared file logging for signal_light PC bridge tools.

Logs go to tools/logs/signal_bridge.log by default (override in signal_bridge_config.json).
Each line includes local timestamp with milliseconds for correlation with device serial logs.
"""

from __future__ import annotations

import logging
import os
import sys
import time
import uuid
from datetime import datetime
from pathlib import Path
from typing import Any

TOOLS_DIR = Path(__file__).resolve().parent
DEFAULT_LOG_DIR = TOOLS_DIR / "logs"
DEFAULT_LOG_FILE = "signal_bridge.log"
CONFIG_PATH = TOOLS_DIR / "signal_bridge_config.json"

_CONFIGURED: set[str] = set()


def load_log_config() -> dict[str, Any]:
    try:
        import json

        if CONFIG_PATH.is_file():
            with CONFIG_PATH.open(encoding="utf-8") as fh:
                cfg = json.load(fh)
                return {
                    "enabled": cfg.get("log_enabled", True),
                    "dir": cfg.get("log_dir", str(DEFAULT_LOG_DIR)),
                    "file": cfg.get("log_file", DEFAULT_LOG_FILE),
                    "console": cfg.get("log_console", False),
                }
    except OSError:
        pass
    return {
        "enabled": True,
        "dir": str(DEFAULT_LOG_DIR),
        "file": DEFAULT_LOG_FILE,
        "console": False,
    }


def log_path(cfg: dict[str, Any] | None = None) -> Path:
    c = cfg or load_log_config()
    base = Path(c["dir"]).expanduser()
    if not base.is_absolute():
        base = TOOLS_DIR.parent / base
    return base.resolve() / str(c["file"])


def new_event_id() -> str:
    return uuid.uuid4().hex[:8]


def format_local_ts(ts: float | None = None) -> str:
    dt = datetime.fromtimestamp(ts if ts is not None else time.time())
    return dt.strftime("%m-%d %H:%M:%S.") + f"{dt.microsecond // 1000:03d}"


class BridgeLogFormatter(logging.Formatter):
    """Device-friendly timestamp prefix: [MM-DD HH:MM:SS.mmm]"""

    def format(self, record: logging.LogRecord) -> str:
        prefix = format_local_ts(record.created)
        body = super().format(record)
        return f"[{prefix}] {body}"


def setup_bridge_logging(
    name: str,
    *,
    verbose: bool = False,
    console: bool | None = None,
    log_cfg: dict[str, Any] | None = None,
) -> logging.Logger:
    """
    Configure logger with rotating-style append file + optional console.
    Safe to call multiple times; handlers attach once per logger name.
    """
    logger = logging.getLogger(name)
    if name in _CONFIGURED:
        logger.setLevel(logging.DEBUG if verbose else logging.INFO)
        return logger

    cfg = log_cfg or load_log_config()
    env_disable = os.environ.get("SIGNAL_BRIDGE_LOG", "").lower() in ("0", "false", "no")
    enabled = bool(cfg.get("enabled", True)) and not env_disable

    level = logging.DEBUG if verbose else logging.INFO
    logger.setLevel(level)
    logger.propagate = False

    fmt = BridgeLogFormatter("%(levelname)s [%(name)s] %(message)s")

    show_console = console if console is not None else bool(cfg.get("console", False)) or verbose
    if show_console:
        sh = logging.StreamHandler(sys.stderr)
        sh.setLevel(level)
        sh.setFormatter(fmt)
        logger.addHandler(sh)

    if enabled:
        path = log_path(cfg)
        path.parent.mkdir(parents=True, exist_ok=True)
        fh = logging.FileHandler(path, encoding="utf-8")
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(fmt)
        logger.addHandler(fh)
        logger.debug("log file: %s", path)

    _CONFIGURED.add(name)
    return logger


def log_push_start(
    logger: logging.Logger,
    *,
    event_id: str,
    status: str,
    transport: str,
    port: str | None = None,
    baud: int | None = None,
    source: str = "bridge",
    extra: str = "",
) -> float:
    parts = [
        f"event={event_id}",
        f"action=push_start",
        f"source={source}",
        f"status={status}",
        f"transport={transport}",
    ]
    if port:
        parts.append(f"port={port}")
    if baud:
        parts.append(f"baud={baud}")
    if extra:
        parts.append(extra)
    logger.info(" | ".join(parts))
    return time.time()


def log_push_end(
    logger: logging.Logger,
    *,
    event_id: str,
    status: str,
    started_at: float,
    ok: bool,
    detail: str = "",
) -> None:
    elapsed_ms = int((time.time() - started_at) * 1000)
    level = logging.INFO if ok else logging.ERROR
    msg = (
        f"event={event_id} | action=push_end | status={status} | "
        f"ok={ok} | elapsed_ms={elapsed_ms}"
    )
    if detail:
        msg += f" | {detail}"
    logger.log(level, msg)
