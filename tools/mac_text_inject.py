"""
Inject UTF-8 text into the frontmost Mac application.

Uses clipboard + Cmd+V for Unicode (ASR Chinese). Falls back to pynput for ASCII.
Requires Accessibility permission for the Python/Terminal process.
"""

from __future__ import annotations

import logging
import platform
import subprocess
import sys

log = logging.getLogger("mac_text_inject")


def inject_text(text: str, *, append_space: bool = True, dry_run: bool = False) -> None:
    payload = text.rstrip("\r\n")
    if append_space and payload and not payload.endswith(" "):
        payload += " "

    if not payload:
        return

    if dry_run:
        log.info("[dry-run] would inject: %r", payload)
        return

    if sys.platform == "darwin":
        _inject_macos(payload)
        return

    if sys.platform == "win32":
        _inject_windows(payload)
        return

    _inject_pynput(payload)


def _inject_windows(text: str) -> None:
    try:
        import pyperclip
        from pynput.keyboard import Controller, Key
    except ImportError as exc:
        raise RuntimeError("Install bridge deps: tools/run_bridge.sh (needs pyperclip, pynput)") from exc

    pyperclip.copy(text)
    kb = Controller()
    with kb.pressed(Key.ctrl):
        kb.press("v")
        kb.release("v")
    log.info("injected via Ctrl+V (Windows) | chars=%d | preview=%r", len(text), text[:80])


def _inject_macos(text: str) -> None:
    """Clipboard paste supports CJK; pynput keystroke does not."""
    subprocess.run(["pbcopy"], input=text.encode("utf-8"), check=True)
    script = 'tell application "System Events" to keystroke "v" using command down'
    try:
        subprocess.run(["osascript", "-e", script], check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as exc:
        detail = (exc.stderr or exc.stdout or str(exc)).strip()
        if "1002" in detail or "not allowed" in detail.lower():
            raise RuntimeError(
                "Mac Accessibility permission missing for keyboard inject. "
                "Open System Settings → Privacy & Security → Accessibility, "
                "enable Cursor (and Terminal if you run scripts there). "
                "macOS may not show a popup for background daemons — add manually, then retry."
            ) from exc
        raise RuntimeError(f"osascript inject failed: {detail}") from exc
    log.info("injected via Cmd+V | chars=%d | preview=%r", len(text), text[:80])


def _inject_pynput(text: str) -> None:
    try:
        from pynput.keyboard import Controller
    except ImportError as exc:
        raise RuntimeError("Install pynput: tools/.venv/bin/pip install pynput") from exc

    Controller().type(text)
    log.info("injected via pynput | chars=%d", len(text))
