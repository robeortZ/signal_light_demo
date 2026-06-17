#!/usr/bin/env python3
"""
Signal bridge — one-shot install + run (Mac / Windows / Linux).

  ./tools/run_bridge.sh          # macOS / Linux (from project root)
  tools\\run_bridge.bat          # Windows

Default transport: WebSocket (LAN). Switch with:
  python3 tools/signal_transport_switch.py uart
  export SIGNAL_BRIDGE_TRANSPORT=uart
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = TOOLS_DIR.parent
REQ_FILE = TOOLS_DIR / "requirements-bridge.txt"


def _venv_python() -> Path:
    if sys.platform == "win32":
        return TOOLS_DIR / ".venv" / "Scripts" / "python.exe"
    return TOOLS_DIR / ".venv" / "bin" / "python3"


def ensure_venv_and_deps() -> Path:
    """Create tools/.venv and install bridge dependencies once."""
    py = _venv_python()
    if not py.is_file():
        print("[bridge] Creating Python venv in tools/.venv …")
        subprocess.check_call([sys.executable, "-m", "venv", str(TOOLS_DIR / ".venv")])

    print("[bridge] Installing / updating dependencies …")
    subprocess.check_call(
        [str(py), "-m", "pip", "install", "-q", "--upgrade", "pip"],
    )
    subprocess.check_call(
        [str(py), "-m", "pip", "install", "-q", "-r", str(REQ_FILE)],
    )
    return py


def _reexec_if_needed() -> None:
    venv_py = _venv_python()
    if not venv_py.is_file():
        venv_py = ensure_venv_and_deps()
    elif Path(sys.executable).resolve() != venv_py.resolve():
        os.environ["PYTHONPATH"] = str(TOOLS_DIR) + (
            os.pathsep + os.environ["PYTHONPATH"] if os.environ.get("PYTHONPATH") else ""
        )
        os.execv(str(venv_py), [str(venv_py), str(Path(__file__).resolve()), *sys.argv[1:]])


def _print_banner(mode: str) -> None:
    plat = "macOS" if sys.platform == "darwin" else "Windows" if sys.platform == "win32" else sys.platform
    print()
    print("=" * 60)
    print(f"  Signal bridge running ({plat})  transport={mode}")
    print("  P29 long-press → cloud Agent → AI reply types into active input")
    print("  P17 click      → voice hotkey | P17 double-click → Enter (enter_hotkey in config)")
    print("  Cursor hooks   → status LED on device")
    print("  Switch mode    → python3 tools/signal_transport_switch.py uart|ws")
    print("  Log file       → tools/logs/signal_bridge.log")
    print("  Stop           → Ctrl+C in this terminal")
    print("=" * 60)
    print()


def main() -> int:
    if "--install-only" in sys.argv:
        ensure_venv_and_deps()
        print("[bridge] Install OK. Run ./tools/run_bridge.sh to start.")
        return 0

    _reexec_if_needed()

    os.environ.setdefault("PYTHONPATH", str(TOOLS_DIR))
    if str(TOOLS_DIR) not in sys.path:
        sys.path.insert(0, str(TOOLS_DIR))

    from bridge_transport import resolve_transport
    from cursor_signal_bridge import load_config, local_lan_ip
    from cursor_signal_keepalive import is_running, run_loop, stop_daemon
    from signal_bridge_log import log_path, setup_bridge_logging

    setup_bridge_logging("signal_bridge_run", console=True)

    cfg = load_config()
    mode = resolve_transport("auto", cfg)

    if mode == "uart" and not cfg.get("uart_port"):
        print("[bridge] ERROR: set uart_port in tools/signal_bridge_config.json")
        return 1

    if is_running():
        print("[bridge] Stopping previous background daemon …")
        stop_daemon(push_idle=False)

    _print_banner(mode)

    if mode == "ws":
        lan_ip = local_lan_ip()
        port = int(cfg.get("ws_listen_port", 8765))
        disc = cfg.get("discovery_enabled", True)
        print(f"[bridge] WebSocket ws://{lan_ip}:{port}/device")
        if disc:
            print(f"[bridge] UDP discovery on :{cfg.get('discovery_port', 8766)} — device finds PC IP automatically")
        else:
            print(f"[bridge] Set firmware SIGNAL_BRIDGE_HOST to {lan_ip} (menuconfig / Kconfig)")
    else:
        print(f"[bridge] UART {cfg.get('uart_port')} @ {cfg.get('uart_baud', 115200)}")

    print(f"[bridge] Log → {log_path()}")
    print()

    try:
        run_loop(with_inline_inject=True)
    except KeyboardInterrupt:
        print("\n[bridge] Stopped.")
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
