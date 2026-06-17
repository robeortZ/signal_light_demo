#!/usr/bin/env python3
"""
PC-side mock client for signal_light_demo.

Sends agent status over UART0 (tal_cli, 115200). Logs to tools/logs/signal_bridge.log.

Usage:
  python3 tools/signal_mock_client.py --port /dev/cu.wchusbserialXXX --status working
  python3 tools/signal_mock_client.py --port /dev/cu.xxx --cycle
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

from signal_bridge_log import log_path, new_event_id, setup_bridge_logging

try:
    import serial
except ImportError:
    print("Install pyserial: pip install pyserial", file=sys.stderr)
    sys.exit(1)

import logging

log = logging.getLogger("mock_client")

CONFIG_PATH = Path(__file__).resolve().parent / "signal_bridge_config.json"

STATUS_MAP = {
    "idle": "SIGNAL idle\n",
    "working": "SIGNAL working\n",
    "attention": "SIGNAL attention\n",
    "urgent": "SIGNAL urgent\n",
    "0": "0",
    "1": "1",
    "2": "2",
    "3": "3",
}

CYCLE = ["idle", "working", "attention", "urgent"]


def load_defaults() -> tuple[str | None, int]:
    if not CONFIG_PATH.is_file():
        return None, 115200
    with CONFIG_PATH.open(encoding="utf-8") as fh:
        cfg = json.load(fh)
    return cfg.get("uart_port"), int(cfg.get("uart_baud", 115200))


def send_status(ser: serial.Serial, name: str) -> None:
    key = name.lower()
    if key not in STATUS_MAP:
        raise ValueError(f"unknown status: {name}")
    payload = STATUS_MAP[key]
    eid = new_event_id()
    t0 = time.time()
    ser.write(payload.encode("ascii"))
    ser.flush()
    elapsed_ms = int((time.time() - t0) * 1000)
    log.info(
        "event=%s | action=mock_send | status=%s | payload=%r | elapsed_ms=%d",
        eid,
        key,
        payload.strip(),
        elapsed_ms,
    )
    print(f"sent: {payload.strip()!r} (event={eid})")


def main() -> int:
    default_port, default_baud = load_defaults()
    parser = argparse.ArgumentParser(description="Signal light UART mock client")
    parser.add_argument("--port", default=default_port, help="Serial port (default from config)")
    parser.add_argument("--baud", type=int, default=default_baud)
    parser.add_argument("--status", help="idle|working|attention|urgent or 0-3")
    parser.add_argument("--cycle", action="store_true", help="Cycle all states every 3s")
    parser.add_argument("--interval", type=float, default=3.0)
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    setup_bridge_logging("mock_client", verbose=args.verbose, console=True)

    if not args.port:
        parser.error("--port required (or set uart_port in signal_bridge_config.json)")

    log.info("mock_client start port=%s baud=%d log=%s", args.port, args.baud, log_path())

    with serial.Serial(args.port, args.baud, timeout=1) as ser:
        if args.cycle:
            print("Cycling states (Ctrl+C to stop)...")
            while True:
                for st in CYCLE:
                    send_status(ser, st)
                    time.sleep(args.interval)
        elif args.status:
            send_status(ser, args.status)
        else:
            parser.print_help()
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
