#!/usr/bin/env python3
"""Switch signal bridge transport (ws / uart) in signal_bridge_config.json."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

CONFIG_PATH = Path(__file__).resolve().parent / "signal_bridge_config.json"
VALID = ("ws", "uart", "ble", "auto")


def load_config() -> dict:
    if CONFIG_PATH.is_file():
        with CONFIG_PATH.open(encoding="utf-8") as fh:
            return json.load(fh)
    return {}


def save_config(cfg: dict) -> None:
    with CONFIG_PATH.open("w", encoding="utf-8") as fh:
        json.dump(cfg, fh, indent=2, ensure_ascii=False)
        fh.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Set or show signal bridge transport mode")
    parser.add_argument("mode", nargs="?", choices=[*VALID, "status"], help="ws | uart | ble | auto")
    args = parser.parse_args()

    cfg = load_config()
    current = str(cfg.get("transport", "ws")).lower()

    if args.mode is None or args.mode == "status":
        print(f"transport={current}")
        print(f"config={CONFIG_PATH}")
        print("override: export SIGNAL_BRIDGE_TRANSPORT=uart")
        return 0

    cfg["transport"] = args.mode
    save_config(cfg)
    print(f"transport set to {args.mode} in {CONFIG_PATH}")
    print("Restart ./tools/run_bridge.sh for the change to take effect.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
