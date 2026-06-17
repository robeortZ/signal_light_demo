#!/usr/bin/env python3
"""
Push agent status to signal_light_demo over BLE (preferred) or UART.

Used by Cursor hooks and manual CLI:
  python3 tools/cursor_signal_bridge.py working
  python3 tools/cursor_signal_bridge.py --transport uart --port /dev/cu.xxx idle
  python3 tools/cursor_signal_bridge.py --scan

Logs (timestamped): tools/logs/signal_bridge.log — correlate with device serial output.
"""

from __future__ import annotations

import argparse
import asyncio
import json
import logging
import re
import sys
import time
from pathlib import Path
from typing import Any, Literal

from signal_bridge_log import (
    load_log_config,
    log_path,
    log_push_end,
    log_push_start,
    new_event_id,
    setup_bridge_logging,
)
from bridge_transport import TransportName, resolve_transport
from uart_ipc import send_to_daemon

log = logging.getLogger("signal_bridge")

STATUS_PAYLOAD = {
    "idle": b"SIGNAL idle\n",
    "working": b"SIGNAL working\n",
    "attention": b"SIGNAL attention\n",
    "urgent": b"SIGNAL urgent\n",
}

StatusName = Literal["idle", "working", "attention", "urgent"]

CONFIG_PATH = Path(__file__).resolve().parent / "signal_bridge_config.json"

# From TAL_BLE_CMD_* in tal_bluetooth_def.h (128-bit write char, V2)
TUYA_SERVICE_UUID = "0000fd50-0000-1000-8000-00805f9b34fb"
TUYA_WRITE_UUID = "d0079b5f-8000-0180-0110-000001000000"


def load_config() -> dict[str, Any]:
    if CONFIG_PATH.is_file():
        with CONFIG_PATH.open(encoding="utf-8") as fh:
            return json.load(fh)
    return {}


def local_lan_ip() -> str:
    """Best-effort LAN IP for device firmware SIGNAL_BRIDGE_HOST."""
    import socket

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"


def _norm_name(value: str | None) -> str:
    return re.sub(r"[^a-z0-9]", "", (value or "").lower())


def _adv_name(adv: Any) -> str:
    for key in ("local_name", "complete_local_name", "short_local_name"):
        name = getattr(adv, key, None)
        if name:
            return str(name)
    if isinstance(adv, dict):
        for key in ("local_name", "complete_local_name", "short_local_name"):
            if adv.get(key):
                return str(adv[key])
    return ""


def _adv_service_uuids(adv: Any) -> list[str]:
    uuids = getattr(adv, "service_uuids", None) or []
    if isinstance(adv, dict):
        uuids = adv.get("service_uuids") or uuids
    return [str(u).lower() for u in uuids]


def _device_matches(
    name: str | None,
    adv: Any,
    *,
    target_name: str,
    service_uuid: str,
    ble_address: str | None,
    address: str,
) -> tuple[bool, str]:
    if ble_address and address.lower() == ble_address.lower():
        return True, "address"

    adv_names = {_norm_name(name), _norm_name(_adv_name(adv))}
    target = _norm_name(target_name)
    if target and any(n and (target in n or n in target) for n in adv_names):
        return True, "name"

    svc = service_uuid.lower().replace("-", "")
    for item in _adv_service_uuids(adv):
        if svc.endswith(item.replace("-", "")) or item.replace("-", "").endswith("fd50"):
            return True, "service_uuid"

    return False, ""


async def discover_device(
    device_name: str,
    service_uuid: str,
    ble_address: str | None,
    timeout: float,
) -> tuple[Any, str]:
    try:
        from bleak import BleakScanner
    except ImportError as exc:
        raise RuntimeError("bleak required: pip install bleak") from exc

    devices = await BleakScanner.discover(timeout=timeout, return_adv=True)
    matches: list[tuple[Any, str, int | None]] = []

    for address, (device, adv) in devices.items():
        ok, reason = _device_matches(
            device.name,
            adv,
            target_name=device_name,
            service_uuid=service_uuid,
            ble_address=ble_address,
            address=address,
        )
        if not ok:
            continue
        rssi = getattr(adv, "rssi", None)
        if isinstance(adv, dict):
            rssi = adv.get("rssi", rssi)
        matches.append((device, reason, rssi))

    if not matches:
        raise RuntimeError(
            f"BLE device not found (name={device_name!r}, service={service_uuid}). "
            "Run: tools/.venv/bin/python3 tools/cursor_signal_bridge.py --scan"
        )

    matches.sort(key=lambda item: item[2] if item[2] is not None else -999, reverse=True)
    device, reason, _ = matches[0]
    label = device.name or _adv_name(devices[device.address][1]) or "unknown"
    log.info("BLE pick %s (%s) via %s", label, device.address, reason)
    return device, reason


def _is_fd50_service(uuid: str) -> bool:
    u = uuid.lower().replace("-", "")
    return u.endswith("0000fd50") or u.endswith("fd50") or "0000fd50" in u


def _find_write_characteristic(client: Any, write_uuid: str | None) -> Any:
    if write_uuid:
        target = write_uuid.lower()
        for service in client.services:
            for char in service.characteristics:
                if char.uuid.lower() == target:
                    return char

    for service in client.services:
        if not _is_fd50_service(service.uuid):
            continue
        for char in service.characteristics:
            props = {p.lower() for p in char.properties}
            if "write" in props or "write-without-response" in props:
                return char

    raise RuntimeError("GATT write characteristic not found on FD50 service")


async def send_ble(
    device_name: str,
    service_uuid: str,
    write_uuid: str | None,
    payload: bytes,
    ble_address: str | None = None,
    timeout: float = 12.0,
) -> None:
    try:
        from bleak import BleakClient
    except ImportError as exc:
        raise RuntimeError("bleak required: pip install bleak") from exc

    device, _ = await discover_device(device_name, service_uuid, ble_address, timeout)
    log.info("BLE connect %s (%s)", device.name or _adv_name({}), device.address)

    async with BleakClient(device, timeout=timeout) as client:
        if not client.is_connected:
            raise RuntimeError("BLE connect failed")

        write_char = _find_write_characteristic(client, write_uuid)
        log.debug("BLE write char %s props=%s", write_char.uuid, write_char.properties)
        await client.write_gatt_char(write_char, payload, response=False)

    log.info("BLE write <= %r", payload.decode("ascii", errors="replace").strip())


async def scan_devices(timeout: float = 8.0) -> None:
    from bleak import BleakScanner

    devices = await BleakScanner.discover(timeout=timeout, return_adv=True)
    print(f"Found {len(devices)} BLE devices (timeout={timeout}s):\n")
    for address, (device, adv) in sorted(devices.items()):
        name = device.name or _adv_name(adv) or "-"
        uuids = ", ".join(_adv_service_uuids(adv)) or "-"
        rssi = getattr(adv, "rssi", None)
        if isinstance(adv, dict):
            rssi = adv.get("rssi", rssi)
        mark = ""
        if _device_matches(
            device.name,
            adv,
            target_name="SignalLight",
            service_uuid=TUYA_SERVICE_UUID,
            ble_address=None,
            address=address,
        )[0]:
            mark = "  <-- signal light candidate"
        print(f"  {address}  {name!r}  RSSI={rssi}  services=[{uuids}]{mark}")


def send_uart(port: str, baud: int, payload: bytes, *, retries: int = 3) -> None:
    if send_to_daemon(payload):
        text = payload.decode("ascii", errors="replace").strip()
        log.info("UART via daemon payload=%r", text)
        return

    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("pyserial required: pip install pyserial") from exc

    t0 = time.time()
    last_exc: Exception | None = None
    for attempt in range(retries):
        try:
            with serial.Serial(port, baud, timeout=2) as ser:
                ser.write(payload)
                ser.flush()
            elapsed_ms = int((time.time() - t0) * 1000)
            text = payload.decode("ascii", errors="replace").strip()
            log.info(
                "UART write port=%s baud=%d payload=%r elapsed_ms=%d attempt=%d",
                port,
                baud,
                text,
                elapsed_ms,
                attempt + 1,
            )
            return
        except serial.SerialException as exc:
            last_exc = exc
            busy = "busy" in str(exc).lower() or getattr(exc, "errno", None) == 16
            if busy and attempt + 1 < retries:
                time.sleep(0.08)
                continue
            raise
    if last_exc is not None:
        raise last_exc


def push_status(
    status: StatusName,
    transport: TransportName = "auto",
    uart_port: str | None = None,
    uart_baud: int | None = None,
    *,
    source: str = "cli",
    event_id: str | None = None,
) -> str:
    """Push status to device. Returns event_id for log correlation."""
    cfg = load_config()
    payload = STATUS_PAYLOAD[status]
    mode = resolve_transport(transport, cfg)
    port = uart_port or cfg.get("uart_port")
    baud = uart_baud or int(cfg.get("uart_baud", 115200))
    ble_name = cfg.get("ble_device_name", "SignalLight")
    svc_uuid = cfg.get("ble_service_uuid", TUYA_SERVICE_UUID)
    write_uuid = cfg.get("ble_write_uuid") or None
    ble_address = cfg.get("ble_address")
    eid = event_id or new_event_id()

    log_push_start(
        log,
        event_id=eid,
        status=status,
        transport=mode,
        port=port if mode in ("auto", "uart") else None,
        baud=baud if mode in ("auto", "uart") else None,
        source=source,
        extra=f"payload={payload.decode('ascii').strip()}",
    )
    t0 = time.time()
    errors: list[str] = []

    try:
        if mode in ("auto", "ws"):
            if send_to_daemon(payload):
                log_push_end(log, event_id=eid, status=status, started_at=t0, ok=True, detail="via=ws")
                return eid
            errors.append("WS: daemon not running or device not connected")
            log.warning("WS ipc failed (start run_bridge.sh?)")
            if mode == "ws":
                raise RuntimeError(
                    "WS bridge not running — start ./tools/run_bridge.sh and ensure device WiFi + bridge host IP"
                )

        if mode in ("auto", "ble"):
            try:
                asyncio.run(
                    send_ble(ble_name, svc_uuid, write_uuid, payload, ble_address=ble_address)
                )
                log_push_end(log, event_id=eid, status=status, started_at=t0, ok=True, detail="via=ble")
                return eid
            except Exception as exc:  # noqa: BLE001
                errors.append(f"BLE: {exc}")
                log.warning("BLE failed: %s", exc)
                if mode == "ble":
                    raise

        if mode in ("auto", "uart"):
            if not port:
                raise RuntimeError("uart_port not set in signal_bridge_config.json")
            send_uart(port, baud, payload)
            log_push_end(log, event_id=eid, status=status, started_at=t0, ok=True, detail="via=uart")
            return eid

        raise RuntimeError(f"unknown transport: {mode}")
    except Exception as exc:  # noqa: BLE001
        log_push_end(
            log,
            event_id=eid,
            status=status,
            started_at=t0,
            ok=False,
            detail=f"error={exc}",
        )
        if errors:
            raise RuntimeError("; ".join(errors + [str(exc)])) from exc
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description="Push Cursor/Claude agent status to T5 signal light")
    parser.add_argument("status", nargs="?", choices=sorted(STATUS_PAYLOAD.keys()))
    parser.add_argument("--transport", choices=["auto", "ble", "uart", "ws"], default="auto")
    parser.add_argument("--port", help="UART port override")
    parser.add_argument("--baud", type=int, default=None)
    parser.add_argument("--scan", action="store_true", help="List nearby BLE devices")
    parser.add_argument(
        "--show-log-path",
        action="store_true",
        help="Print log file path and exit",
    )
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    setup_bridge_logging("signal_bridge", verbose=args.verbose)

    if args.show_log_path:
        print(log_path(load_log_config()))
        return 0

    if args.scan:
        try:
            asyncio.run(scan_devices())
        except Exception as exc:  # noqa: BLE001
            log.error("%s", exc)
            return 1
        return 0

    if not args.status:
        parser.error("status is required unless --scan is used")

    try:
        eid = push_status(args.status, args.transport, args.port, args.baud, source="cli")
        log.info("done event=%s log=%s", eid, log_path())
    except Exception as exc:  # noqa: BLE001
        log.error("%s", exc)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
