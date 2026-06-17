#!/usr/bin/env python3
"""UDP LAN beacon so the device can discover the PC bridge IP automatically."""

from __future__ import annotations

import logging
import socket
import threading
import time

log = logging.getLogger("signal_discovery")

BEACON_PREFIX = "SIGNAL_BRIDGE"


def format_beacon(ip: str, ws_port: int) -> bytes:
    """Build discovery datagram payload."""
    return f"{BEACON_PREFIX} ip={ip} port={ws_port}\n".encode("ascii")


def run_discovery_broadcaster(
    *,
    lan_ip: str,
    ws_port: int,
    discovery_port: int = 8766,
    interval_s: float = 2.0,
    running: threading.Event,
) -> None:
    """Broadcast bridge endpoint on LAN until running is cleared."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        payload = format_beacon(lan_ip, ws_port)
        log.info(
            "discovery beacon UDP :%d every %.1fs -> %s (ws port %d)",
            discovery_port,
            interval_s,
            lan_ip,
            ws_port,
        )
        while running.is_set():
            try:
                sock.sendto(payload, ("255.255.255.255", discovery_port))
            except OSError as exc:
                log.warning("discovery send failed: %s", exc)
            time.sleep(interval_s)
    finally:
        sock.close()
