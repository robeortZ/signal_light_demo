#!/usr/bin/env python3
"""
UART daemon: owns the serial port for the whole Cursor agent session.

- Writes SIGNAL status (keepalive + hook IPC via unix socket)
- Reads ASR lines from device and injects text into the active Mac app
"""

from __future__ import annotations

import argparse
import logging
import os
import queue
import re
import signal
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

from cursor_signal_bridge import STATUS_PAYLOAD, load_config, local_lan_ip, push_status
from bridge_transport import resolve_transport
from mac_text_inject import inject_text
from signal_bridge_log import log_path, setup_bridge_logging
from signal_discovery import run_discovery_broadcaster
from signal_ws_server import WsBridgeHub, run_ws_server
from uart_ipc import SOCK_PATH, parse_signal_status, send_to_daemon

try:
    from mac_hotkey_inject import trigger_enter_key, trigger_voice_hotkey
except ImportError:
    trigger_enter_key = None  # type: ignore[assignment,misc]
    trigger_voice_hotkey = None  # type: ignore[assignment,misc]

TOOLS_DIR = Path(__file__).resolve().parent
PID_FILE = TOOLS_DIR / "logs" / "signal_keepalive.pid"
STATE_FILE = TOOLS_DIR / "logs" / "signal_keepalive_status.txt"
if os.name == "nt":
    VENV_PYTHON = TOOLS_DIR / ".venv" / "Scripts" / "python.exe"
else:
    VENV_PYTHON = TOOLS_DIR / ".venv" / "bin" / "python3"

log = logging.getLogger("signal_keepalive")


def _bridge_python() -> str:
    if VENV_PYTHON.is_file():
        return str(VENV_PYTHON)
    return sys.executable


def _port_holders(port: str) -> list[str]:
    """Return brief process descriptions holding the UART device."""
    if sys.platform == "win32":
        return []
    import subprocess

    try:
        out = subprocess.check_output(["lsof", port], stderr=subprocess.DEVNULL, text=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []
    lines = out.strip().splitlines()
    if len(lines) <= 1:
        return []
    holders: list[str] = []
    for row in lines[1:]:
        parts = row.split()
        if len(parts) >= 2:
            holders.append(f"{parts[0]} pid={parts[1]}")
    return holders


def _tcp_listeners(port: int) -> list[tuple[int, str]]:
    """Return (pid, command) for processes listening on a TCP port."""
    if sys.platform == "win32":
        return []
    try:
        out = subprocess.check_output(
            ["lsof", "-nP", f"-iTCP:{port}", "-sTCP:LISTEN"],
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []
    listeners: list[tuple[int, str]] = []
    for row in out.strip().splitlines()[1:]:
        parts = row.split()
        if len(parts) < 2:
            continue
        try:
            pid = int(parts[1])
        except ValueError:
            continue
        listeners.append((pid, parts[0]))
    return listeners


def _keepalive_worker_pids() -> list[int]:
    """PIDs of background/foreground keepalive workers (--run)."""
    if sys.platform == "win32":
        return []
    try:
        out = subprocess.check_output(
            ["pgrep", "-f", "cursor_signal_keepalive.py --run"],
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []
    pids: list[int] = []
    for line in out.strip().splitlines():
        try:
            pids.append(int(line.strip()))
        except ValueError:
            continue
    return pids


def _wait_port_free(port: int, *, timeout: float = 3.0) -> bool:
    """Wait until no process listens on TCP port."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not _tcp_listeners(port):
            return True
        time.sleep(0.1)
    return not _tcp_listeners(port)


def _stop_keepalive_workers(*, port: int | None = None, exclude_pid: int | None = None) -> None:
    """Stop all keepalive workers and release the WS listen port if given."""
    targets: set[int] = set(_keepalive_worker_pids())
    if PID_FILE.is_file():
        try:
            targets.add(int(PID_FILE.read_text(encoding="utf-8").strip()))
        except ValueError:
            PID_FILE.unlink(missing_ok=True)

    if port is not None:
        for pid, _cmd in _tcp_listeners(port):
            targets.add(pid)

    for pid in sorted(targets):
        if exclude_pid is not None and pid == exclude_pid:
            continue
        try:
            os.kill(pid, signal.SIGTERM)
            log.info("keepalive stop signal pid=%d", pid)
        except OSError as exc:
            log.debug("keepalive stop pid=%d: %s", pid, exc)

    PID_FILE.unlink(missing_ok=True)

    if port is not None:
        if not _wait_port_free(port):
            holders = ", ".join(f"pid={p}" for p, _ in _tcp_listeners(port))
            log.warning("port %d still in use after stop: %s", port, holders)


def _ensure_ws_port_free(port: int, *, fatal: bool = False) -> bool:
    """Kill stale bridge listeners and wait for the WS port."""
    my_pid = os.getpid()
    listeners = [(pid, cmd) for pid, cmd in _tcp_listeners(port) if pid != my_pid]
    if listeners:
        log.warning(
            "ws port %d busy (%s), stopping stale bridge process(es) …",
            port,
            ", ".join(f"pid={p}" for p, _ in listeners),
        )
        _stop_keepalive_workers(port=port, exclude_pid=my_pid)

    if _wait_port_free(port):
        return True

    holders = ", ".join(f"pid={p}" for p, _ in _tcp_listeners(port))
    log.error("ws port %d still in use: %s", port, holders)
    if fatal:
        print(f"\n[bridge] ERROR: TCP port {port} is in use ({holders}).")
        print(f"  Try: lsof -nP -iTCP:{port} -sTCP:LISTEN")
        print("  Or change ws_listen_port in tools/signal_bridge_config.json\n")
        sys.exit(1)
    return False


def _warn_port_conflict(port: str, self_pid: int | None = None) -> None:
    holders = _port_holders(port)
    others = []
    for item in holders:
        if self_pid is not None and f"pid={self_pid}" in item:
            continue
        others.append(item)
    if not others:
        return
    log.error(
        "uart port %s is also open by: %s | only ONE process may use bridge UART; "
        "stop pixel-agent-bridge / IDE serial monitor on this port, then restart keepalive",
        port,
        ", ".join(others),
    )


ASR_PREFIX = re.compile(r"^ASR[:\s]+", re.IGNORECASE)
VOICE_LINE_RE = re.compile(r"^VOICE\s+(down|up)\s*$", re.IGNORECASE)
KEY_LINE_RE = re.compile(r"^KEY\s+(enter|return)\s*$", re.IGNORECASE)


def _hotkey_enabled(mode: str) -> bool:
    return mode in ("hotkey", "both")


def _asr_inject_enabled(mode: str) -> bool:
    return mode in ("asr", "both")


def parse_asr_line(raw: str) -> str | None:
    line = raw.strip()
    if not line.upper().startswith("ASR"):
        return None
    text = ASR_PREFIX.sub("", line, count=1).strip()
    return text if text else None


def set_keepalive_status(status: str) -> None:
    STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
    STATE_FILE.write_text(status, encoding="utf-8")


def get_keepalive_status() -> str:
    if not STATE_FILE.is_file():
        return "idle"
    value = STATE_FILE.read_text(encoding="utf-8").strip()
    return value if value in STATUS_PAYLOAD else "idle"


def ensure_running() -> None:
    if not is_running():
        start_daemon()


def _pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def is_running() -> bool:
    if not PID_FILE.is_file():
        return False
    try:
        pid = int(PID_FILE.read_text(encoding="utf-8").strip())
    except ValueError:
        PID_FILE.unlink(missing_ok=True)
        return False
    if _pid_alive(pid):
        return True
    PID_FILE.unlink(missing_ok=True)
    return False


def start_daemon() -> int:
    if is_running():
        log.info("keepalive already running pid=%s", PID_FILE.read_text(encoding="utf-8").strip())
        return 0

    cfg = load_config()
    if resolve_transport("auto", cfg) == "ws":
        listen_port = int(cfg.get("ws_listen_port", 8765))
        if _tcp_listeners(listen_port):
            log.info("keepalive ws port %d already served (foreground bridge?)", listen_port)
            return 0

    set_keepalive_status("working")
    PID_FILE.parent.mkdir(parents=True, exist_ok=True)
    stderr_path = log_path()
    proc = subprocess.Popen(
        [_bridge_python(), str(Path(__file__).resolve()), "--run"],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=open(stderr_path, "a", encoding="utf-8"),
        start_new_session=True,
        cwd=str(TOOLS_DIR.parent),
        env={**os.environ, "PYTHONPATH": str(TOOLS_DIR)},
    )
    PID_FILE.write_text(str(proc.pid), encoding="utf-8")
    log.info("uart daemon started pid=%d log=%s", proc.pid, stderr_path)
    return 0


def stop_daemon(*, push_idle: bool = True) -> int:
    if push_idle:
        send_to_daemon(STATUS_PAYLOAD["idle"])

    cfg = load_config()
    ws_port = int(cfg.get("ws_listen_port", 8765)) if resolve_transport("auto", cfg) == "ws" else None

    if not PID_FILE.is_file() and not _keepalive_worker_pids() and (ws_port is None or not _tcp_listeners(ws_port)):
        log.debug("keepalive stop: not running")
        if push_idle:
            try:
                push_status("idle", source="keepalive_stop")
            except Exception as exc:  # noqa: BLE001
                log.warning("idle push failed: %s", exc)
        return 0

    _stop_keepalive_workers(port=ws_port)
    set_keepalive_status("idle")

    if push_idle:
        try:
            push_status("idle", source="keepalive_stop")
        except Exception as exc:  # noqa: BLE001
            log.warning("idle push failed: %s", exc)
    return 0


def _write_serial(ser, serial_lock: threading.Lock, payload: bytes) -> None:
    with serial_lock:
        ser.write(payload)
        ser.flush()


def _open_serial(port: str, baud: int):
    import serial

    return serial.Serial(port, baud, timeout=0.2)


def _reopen_serial(
    port: str,
    baud: int,
    serial_lock: threading.Lock,
    ser_holder: list,
) -> bool:
    import serial

    with serial_lock:
        old = ser_holder[0]
        try:
            if old is not None and getattr(old, "is_open", False):
                old.close()
        except Exception:  # noqa: BLE001
            pass
        try:
            ser_holder[0] = serial.Serial(port, baud, timeout=0.2)
            log.info("uart reopened | port=%s baud=%d", port, baud)
            return True
        except serial.SerialException as exc:
            log.warning("uart reopen failed port=%s: %s", port, exc)
            ser_holder[0] = old
            return False


def _serial_io_ok(ser) -> bool:
    try:
        return ser is not None and bool(getattr(ser, "is_open", False))
    except Exception:  # noqa: BLE001
        return False


def parse_voice_edge(raw: str) -> str | None:
    match = VOICE_LINE_RE.match(raw.strip())
    if not match:
        return None
    return match.group(1).lower()


def parse_key_line(raw: str) -> str | None:
    match = KEY_LINE_RE.match(raw.strip())
    if not match:
        return None
    return match.group(1).lower()


def _deliver_key(key: str) -> None:
    if key not in ("enter", "return"):
        log.warning("action=key_skip | unknown key=%s", key)
        return
    cfg = load_config()
    spec = str(cfg.get("enter_hotkey", "enter"))
    log.info("action=key_rx | key=%s spec=%s", key, spec)
    if trigger_enter_key is None:
        log.error("action=key_fail | enter module unavailable")
        return
    try:
        trigger_enter_key(spec=spec, dry_run=False)
        log.info("action=key_enter | spec=%s", spec)
    except Exception as exc:  # noqa: BLE001
        log.error(
            "action=key_fail | key=%s error=%s | hint=grant Accessibility, restart ./tools/run_bridge.sh",
            key,
            exc,
        )


def _deliver_voice_hotkey(edge: str) -> None:
    cfg = load_config()
    mode = str(cfg.get("voice_input_mode", "both"))
    if not _hotkey_enabled(mode):
        log.info("action=voice_skip | edge=%s mode=%s (need hotkey or both)", edge, mode)
        return

    hotkey = str(cfg.get("voice_hotkey", "ctrl+fn"))
    if sys.platform == "win32":
        hotkey = str(cfg.get("voice_hotkey_win", cfg.get("voice_hotkey", "ctrl+shift+space")))
    log.info("action=voice_rx | edge=%s hotkey=%s", edge, hotkey)

    phase = "down" if edge == "down" else "up"

    if trigger_voice_hotkey is None:
        log.error("action=voice_fail | hotkey module unavailable")
        return
    try:
        trigger_voice_hotkey(hotkey, phase=phase, dry_run=False)
        log.info("action=voice_hotkey | edge=%s", edge)
    except Exception as exc:  # noqa: BLE001
        log.error(
            "action=voice_fail | edge=%s error=%s | hint=run tools/run_bridge.sh in Cursor terminal",
            edge,
            exc,
        )


def _deliver_asr(text: str, *, append_space: bool) -> None:
    try:
        inject_text(text, append_space=append_space, dry_run=False)
        log.info("action=asr_injected | text=%r", text)
    except Exception as exc:  # noqa: BLE001
        log.error(
            "action=asr_inject_fail | error=%s | hint=grant Accessibility (Mac) or run as foreground",
            exc,
        )


def _read_loop(
    ser_holder: list,
    serial_lock: threading.Lock,
    running: threading.Event,
    *,
    port: str,
    baud: int,
    append_space: bool,
) -> None:
    byte_buf = b""
    while running.is_set():
        ser = ser_holder[0]
        if not _serial_io_ok(ser):
            if not _reopen_serial(port, baud, serial_lock, ser_holder):
                time.sleep(1.0)
                continue
            ser = ser_holder[0]
            byte_buf = b""

        try:
            with serial_lock:
                chunk = ser.read(256)
        except Exception as exc:  # noqa: BLE001
            log.warning("uart read failed: %s — will reopen", exc)
            _reopen_serial(port, baud, serial_lock, ser_holder)
            time.sleep(0.5)
            continue

        if not chunk:
            time.sleep(0.05)
            continue

        byte_buf += chunk
        while b"\n" in byte_buf:
            line_b, byte_buf = byte_buf.split(b"\n", 1)
            try:
                line = line_b.decode("utf-8")
            except UnicodeDecodeError:
                log.debug("action=serial_skip | reason=invalid_utf8_line")
                continue
            voice_edge = parse_voice_edge(line)
            if voice_edge is not None:
                _deliver_voice_hotkey(voice_edge)
                continue
            key = parse_key_line(line)
            if key is not None:
                _deliver_key(key)
                continue
            text = parse_asr_line(line)
            if not text:
                continue
            if not _asr_inject_enabled(str(load_config().get("voice_input_mode", "both"))):
                log.info("action=asr_skip | mode=hotkey-only (set voice_input_mode=asr or both)")
                continue
            log.info("action=asr_rx | text=%r", text)
            _deliver_asr(text, append_space=append_space)


def _socket_server(write_queue: queue.SimpleQueue[bytes], running: threading.Event) -> None:
    SOCK_PATH.parent.mkdir(parents=True, exist_ok=True)
    if SOCK_PATH.exists():
        SOCK_PATH.unlink()

    if hasattr(socket, "AF_UNIX"):
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(str(SOCK_PATH))
    else:
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("127.0.0.1", 19876))
    server.listen(8)
    log.info("uart daemon socket %s", SOCK_PATH if hasattr(socket, "AF_UNIX") else "127.0.0.1:19876")

    try:
        while running.is_set():
            server.settimeout(1.0)
            try:
                conn, _ = server.accept()
            except (TimeoutError, socket.timeout, OSError):
                if not running.is_set():
                    break
                continue
            try:
                data = conn.recv(4096)
                if data:
                    write_queue.put(data)
            finally:
                conn.close()
    finally:
        server.close()
        if SOCK_PATH.exists():
            SOCK_PATH.unlink()


def _on_device_line(line: str, *, append_space: bool) -> None:
    voice_edge = parse_voice_edge(line)
    if voice_edge is not None:
        _deliver_voice_hotkey(voice_edge)
        return
    key = parse_key_line(line)
    if key is not None:
        _deliver_key(key)
        return
    text = parse_asr_line(line)
    if not text:
        return
    if not _asr_inject_enabled(str(load_config().get("voice_input_mode", "both"))):
        log.info("action=asr_skip | mode=hotkey-only (set voice_input_mode=asr or both)")
        return
    log.info("action=asr_rx | text=%r", text)
    _deliver_asr(text, append_space=append_space)


def run_loop(*, with_inline_inject: bool = False) -> None:
    setup_bridge_logging("signal_keepalive", console=with_inline_inject)
    cfg = load_config()
    mode = resolve_transport("auto", cfg)
    if mode == "ws":
        _run_ws_loop(with_inline_inject=with_inline_inject)
    else:
        _run_uart_loop(with_inline_inject=with_inline_inject)


def _run_uart_loop(*, with_inline_inject: bool = False) -> None:
    setup_bridge_logging("signal_keepalive", console=with_inline_inject)
    cfg = load_config()
    interval_s = float(cfg.get("keepalive_interval_ms", 800)) / 1000.0
    port = cfg.get("uart_port")
    baud = int(cfg.get("uart_baud", 115200))
    append_space = bool(cfg.get("asr_append_space", True))

    if not port:
        log.error("keepalive: uart_port not set")
        sys.exit(1)

    others = [h for h in _port_holders(port)]
    if others:
        msg = f"UART port busy ({port}): {', '.join(others)}"
        log.error("%s — stop pixel-agent-bridge / other serial tools first", msg)
        if with_inline_inject:
            print(f"\n[bridge] ERROR: {msg}\n")
            sys.exit(1)

    try:
        import serial
    except ImportError:
        log.error("pyserial required")
        sys.exit(1)

    running = threading.Event()
    running.set()
    serial_lock = threading.Lock()
    write_queue: queue.SimpleQueue[bytes] = queue.SimpleQueue()
    ser_holder = [_open_serial(port, baud)]

    def _on_sigterm(_signum: int, _frame: object) -> None:
        running.clear()
        try:
            _write_serial(ser_holder[0], serial_lock, STATUS_PAYLOAD["idle"])
        except Exception:  # noqa: BLE001
            pass
        try:
            ser_holder[0].close()
        except Exception:  # noqa: BLE001
            pass
        sys.exit(0)

    signal.signal(signal.SIGTERM, _on_sigterm)

    PID_FILE.parent.mkdir(parents=True, exist_ok=True)
    PID_FILE.write_text(str(os.getpid()), encoding="utf-8")

    threading.Thread(
        target=_read_loop,
        args=(ser_holder, serial_lock, running),
        kwargs={"port": port, "baud": baud, "append_space": append_space},
        daemon=True,
        name="uart_asr_read",
    ).start()
    threading.Thread(
        target=_socket_server,
        args=(write_queue, running),
        daemon=True,
        name="uart_ipc",
    ).start()

    log.info(
        "uart daemon ready | pid=%d port=%s baud=%d keepalive=%.3fs inject=%s",
        os.getpid(),
        port,
        baud,
        interval_s,
        "inline" if with_inline_inject else "background",
    )

    if with_inline_inject:
        print("[bridge] Ready — focus an input box, then use P29 (ASR) or P17 (hotkey).\n")

    try:
        while running.is_set():
            while True:
                try:
                    payload = write_queue.get_nowait()
                except queue.Empty:
                    break
                status = parse_signal_status(payload)
                if status and status in STATUS_PAYLOAD:
                    set_keepalive_status(status)
                try:
                    _write_serial(ser_holder[0], serial_lock, payload)
                    log.debug("uart tx %r", payload.decode("ascii", errors="replace").strip())
                except Exception as exc:  # noqa: BLE001
                    log.warning("uart tx failed: %s", exc)
                    _reopen_serial(port, baud, serial_lock, ser_holder)

            status = get_keepalive_status()
            if status != "idle":
                payload = STATUS_PAYLOAD[status]  # type: ignore[literal-required]
                try:
                    _write_serial(ser_holder[0], serial_lock, payload)
                except Exception as exc:  # noqa: BLE001
                    log.warning("keepalive tick failed status=%s: %s", status, exc)
                    _reopen_serial(port, baud, serial_lock, ser_holder)

            time.sleep(interval_s)
    finally:
        PID_FILE.unlink(missing_ok=True)
        try:
            ser_holder[0].close()
        except Exception:  # noqa: BLE001
            pass


def _run_ws_loop(*, with_inline_inject: bool = False) -> None:
    import asyncio

    cfg = load_config()
    interval_s = float(cfg.get("keepalive_interval_ms", 800)) / 1000.0
    listen_host = str(cfg.get("ws_listen_host", "0.0.0.0"))
    listen_port = int(cfg.get("ws_listen_port", 8765))
    discovery_port = int(cfg.get("discovery_port", 8766))
    discovery_interval_s = float(cfg.get("discovery_interval_ms", 2000)) / 1000.0
    discovery_enabled = bool(cfg.get("discovery_enabled", True))
    append_space = bool(cfg.get("asr_append_space", True))
    lan_ip = local_lan_ip()

    _ensure_ws_port_free(listen_port, fatal=with_inline_inject)

    running = threading.Event()
    running.set()
    write_queue: queue.SimpleQueue[bytes] = queue.SimpleQueue()
    stop_ws = threading.Event()

    hub = WsBridgeHub(lambda line: _on_device_line(line, append_space=append_space))

    def _ws_thread() -> None:
        try:
            asyncio.run(run_ws_server(hub, listen_host, listen_port, stop_ws))
        except Exception as exc:  # noqa: BLE001
            log.error("ws server exited: %s", exc)

    def _on_sigterm(_signum: int, _frame: object) -> None:
        running.clear()
        stop_ws.set()
        hub.send_bytes(STATUS_PAYLOAD["idle"])
        sys.exit(0)

    signal.signal(signal.SIGTERM, _on_sigterm)

    PID_FILE.parent.mkdir(parents=True, exist_ok=True)
    PID_FILE.write_text(str(os.getpid()), encoding="utf-8")

    threading.Thread(target=_ws_thread, daemon=True, name="ws_server").start()
    threading.Thread(
        target=_socket_server,
        args=(write_queue, running),
        daemon=True,
        name="bridge_ipc",
    ).start()

    if discovery_enabled:
        threading.Thread(
            target=run_discovery_broadcaster,
            kwargs={
                "lan_ip": lan_ip,
                "ws_port": listen_port,
                "discovery_port": discovery_port,
                "interval_s": discovery_interval_s,
                "running": running,
            },
            daemon=True,
            name="discovery_beacon",
        ).start()

    log.info(
        "ws daemon ready | pid=%d listen=%s:%d lan_ip=%s discovery=%s:%d keepalive=%.3fs",
        os.getpid(),
        listen_host,
        listen_port,
        lan_ip,
        "on" if discovery_enabled else "off",
        discovery_port,
        interval_s,
    )

    if with_inline_inject:
        print(f"[bridge] WS ws://{lan_ip}:{listen_port}/device")
        if discovery_enabled:
            print(f"[bridge] UDP discovery beacon -> :{discovery_port} (device auto-finds this PC)")
        else:
            print(f"[bridge] Set firmware SIGNAL_BRIDGE_HOST to {lan_ip} (discovery disabled)")
        print("[bridge] Ready — focus an input box, then use P29 (ASR) or P17 (hotkey).\n")

    try:
        while running.is_set():
            while True:
                try:
                    payload = write_queue.get_nowait()
                except queue.Empty:
                    break
                status = parse_signal_status(payload)
                if status and status in STATUS_PAYLOAD:
                    set_keepalive_status(status)
                if not hub.send_bytes(payload):
                    log.debug("ws tx skipped (no device connected): %r",
                              payload.decode("ascii", errors="replace").strip())
                else:
                    log.debug("ws tx %r", payload.decode("ascii", errors="replace").strip())

            status = get_keepalive_status()
            if status != "idle" and hub.is_connected():
                payload = STATUS_PAYLOAD[status]  # type: ignore[literal-required]
                hub.send_bytes(payload)

            time.sleep(interval_s)
    finally:
        stop_ws.set()
        PID_FILE.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="UART daemon: status keepalive + ASR inject")
    parser.add_argument("command", nargs="?", choices=["start", "stop", "status", "inject-asr"])
    parser.add_argument("text", nargs="?", help="ASR text for inject-asr (simulate device ASR line)")
    parser.add_argument("--run", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()

    if args.run:
        run_loop()
        return 0

    setup_bridge_logging("signal_keepalive")

    if args.command == "inject-asr":
        cfg = load_config()
        text = args.text
        if not text:
            parser.error("inject-asr requires text argument")
        append_space = bool(cfg.get("asr_append_space", True))
        log.info("action=asr_simulate | text=%r", text)
        _deliver_asr(text, append_space=append_space)
        return 0

    if args.command == "start":
        return start_daemon()
    if args.command == "stop":
        return stop_daemon()
    if args.command == "status":
        print("running" if is_running() else "stopped")
        return 0

    parser.error("command required: start | stop | status")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
