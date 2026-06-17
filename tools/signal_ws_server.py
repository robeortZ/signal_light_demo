#!/usr/bin/env python3
"""WebSocket server for signal_light_demo LAN bridge (device connects to /device)."""

from __future__ import annotations

import asyncio
import logging
import threading
from typing import Callable

log = logging.getLogger("signal_ws_server")


class WsBridgeHub:
    """Single-device WebSocket hub; thread-safe send from sync code."""

    def __init__(self, on_line: Callable[[str], None]) -> None:
        self._on_line = on_line
        self._client = None
        self._lock = threading.Lock()
        self._loop: asyncio.AbstractEventLoop | None = None

    def attach_loop(self, loop: asyncio.AbstractEventLoop) -> None:
        self._loop = loop

    async def _handler(self, websocket) -> None:
        path = "/device"
        request = getattr(websocket, "request", None)
        if request is not None:
            path = getattr(request, "path", path) or path
        elif hasattr(websocket, "path"):
            path = websocket.path or path

        if path != "/device":
            log.warning("ws reject path=%s", path)
            await websocket.close()
            return

        with self._lock:
            if self._client is not None:
                log.warning("ws replacing previous client")
            self._client = websocket

        peer = getattr(websocket, "remote_address", None)
        log.info("ws device connected peer=%s", peer)

        try:
            async for message in websocket:
                if isinstance(message, bytes):
                    try:
                        text = message.decode("utf-8")
                    except UnicodeDecodeError:
                        log.debug("ws skip non-utf8 (%d bytes)", len(message))
                        continue
                else:
                    text = message
                for line in text.splitlines():
                    line = line.strip()
                    if line:
                        if line.upper().startswith("KEY "):
                            log.info("ws rx | %s", line)
                        self._on_line(line)
        except Exception as exc:  # noqa: BLE001
            log.warning("ws client error: %s", exc)
        finally:
            with self._lock:
                if self._client is websocket:
                    self._client = None
            log.info("ws device disconnected")

    def send_bytes(self, payload: bytes) -> bool:
        if self._loop is None:
            return False
        with self._lock:
            client = self._client
        if client is None:
            return False

        async def _send() -> None:
            await client.send(payload.decode("ascii"))

        future = asyncio.run_coroutine_threadsafe(_send(), self._loop)
        try:
            future.result(timeout=3.0)
            return True
        except Exception as exc:  # noqa: BLE001
            log.warning("ws send failed: %s", exc)
            return False

    def is_connected(self) -> bool:
        with self._lock:
            return self._client is not None


async def run_ws_server(
    hub: WsBridgeHub,
    host: str,
    port: int,
    stop_event: threading.Event,
) -> None:
    import websockets

    hub.attach_loop(asyncio.get_running_loop())
    log.info("ws listen ws://%s:%d/device", host, port)

    async with websockets.serve(
        hub._handler,
        host,
        port,
        ping_interval=20,
        ping_timeout=60,
    ):
        while not stop_event.is_set():
            await asyncio.sleep(0.25)
