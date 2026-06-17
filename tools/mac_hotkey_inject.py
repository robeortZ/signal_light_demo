"""
Simulate Mac keyboard shortcuts (e.g. WeChat voice-to-text Ctrl+Fn hold).

pynput cannot synthesize Fn on macOS (Key.fn missing). Combos containing fn use
Quartz CGEvent with virtual key codes instead.
"""

from __future__ import annotations

import logging
import sys
import time

log = logging.getLogger("mac_hotkey_inject")

_HELD: dict[str, list[int]] = {}

# Carbon Events.h virtual key codes
_DARWIN_VK = {
    "ctrl": 0x3B,
    "control": 0x3B,
    "cmd": 0x37,
    "command": 0x37,
    "fn": 0x3F,
    "function": 0x3F,
    "shift": 0x38,
    "alt": 0x3A,
    "option": 0x3A,
}


def _parse_parts(spec: str) -> list[str]:
    parts = [part.strip().lower() for part in spec.replace("+", " ").split() if part.strip()]
    if not parts:
        raise ValueError("voice_hotkey is empty")
    return parts


def _parts_to_keycodes(parts: list[str]) -> list[int]:
    keycodes: list[int] = []
    for part in parts:
        vk = _DARWIN_VK.get(part)
        if vk is None:
            raise ValueError(f"unknown or unsupported hotkey part on macOS: {part!r}")
        keycodes.append(vk)
    return keycodes


def _needs_quartz(parts: list[str]) -> bool:
    return any(part in ("fn", "function") for part in parts)


def _quartz_post(keycode: int, down: bool) -> None:
    from Quartz import CGEventCreateKeyboardEvent, CGEventPost, kCGHIDEventTap

    event = CGEventCreateKeyboardEvent(None, keycode, down)
    CGEventPost(kCGHIDEventTap, event)


def _trigger_quartz(keycodes: list[int], *, phase: str, hotkey: str) -> None:
    if phase == "toggle":
        log.info("action=hotkey_press | combo=%s backend=quartz", hotkey)
        for vk in keycodes:
            _quartz_post(vk, True)
            time.sleep(0.02)
        time.sleep(0.05)
        for vk in reversed(keycodes):
            _quartz_post(vk, False)
            time.sleep(0.02)
        log.info("action=hotkey_done | combo=%s", hotkey)
        return

    if phase == "down":
        if hotkey in _HELD:
            log.debug("action=hotkey_down_skip | combo=%s already held", hotkey)
            return
        log.info("action=hotkey_down | combo=%s backend=quartz", hotkey)
        for vk in keycodes:
            _quartz_post(vk, True)
            time.sleep(0.02)
        _HELD[hotkey] = keycodes
        return

    if phase == "up":
        held = _HELD.pop(hotkey, None)
        if held is None:
            log.debug("action=hotkey_up_skip | combo=%s not held", hotkey)
            return
        log.info("action=hotkey_up | combo=%s backend=quartz", hotkey)
        for vk in reversed(held):
            _quartz_post(vk, False)
            time.sleep(0.02)
        return

    raise ValueError(f"unknown hotkey phase: {phase!r}")


def _parse_pynput_hotkey(spec: str) -> tuple[list[object], list[object]]:
    from pynput.keyboard import Key

    key_map = {
        "ctrl": Key.ctrl,
        "control": Key.ctrl,
        "cmd": Key.cmd,
        "command": Key.cmd,
        "shift": Key.shift,
        "alt": Key.alt,
        "option": Key.alt,
        "space": Key.space,
        "win": Key.cmd,
        "windows": Key.cmd,
    }

    modifiers: list = []
    taps: list = []
    for part in _parse_parts(spec):
        key = key_map.get(part)
        if key is None:
            if len(part) == 1:
                taps.append(part)
            else:
                raise ValueError(f"unknown hotkey part: {part!r}")
            continue
        modifiers.append(key)
    return modifiers, taps


def _trigger_pynput(
    modifiers: list,
    taps: list,
    *,
    phase: str,
    hotkey: str,
) -> None:
    from pynput.keyboard import Controller

    controller = Controller()

    if phase == "toggle":
        log.info("action=hotkey_press | combo=%s backend=pynput", hotkey)
        for mod in modifiers:
            controller.press(mod)
        for key in taps:
            controller.press(key)
            controller.release(key)
        for mod in reversed(modifiers):
            controller.release(mod)
        log.info("action=hotkey_done | combo=%s", hotkey)
        return

    if phase == "down":
        if hotkey in _HELD:
            log.debug("action=hotkey_down_skip | combo=%s already held", hotkey)
            return
        log.info("action=hotkey_down | combo=%s backend=pynput", hotkey)
        for mod in modifiers:
            controller.press(mod)
        for key in taps:
            controller.press(key)
        _HELD[hotkey] = []  # marker only; pynput path stores via separate dict below
        _HELD[f"{hotkey}__pynput"] = {"mods": modifiers, "taps": taps}
        return

    if phase == "up":
        held = _HELD.pop(f"{hotkey}__pynput", None)
        _HELD.pop(hotkey, None)
        if held is None:
            log.debug("action=hotkey_up_skip | combo=%s not held", hotkey)
            return
        log.info("action=hotkey_up | combo=%s backend=pynput", hotkey)
        for key in reversed(held["taps"]):
            controller.release(key)
        for mod in reversed(held["mods"]):
            controller.release(mod)
        return

    raise ValueError(f"unknown hotkey phase: {phase!r}")


def trigger_voice_hotkey(
    hotkey: str = "ctrl+fn",
    *,
    phase: str = "toggle",
    dry_run: bool = False,
) -> None:
    """
    Simulate configured hotkey (Mac: Quartz for Fn combos; Windows/Linux: pynput).
    """
    if dry_run:
        log.info("[dry-run] would hotkey phase=%s combo=%s", phase, hotkey)
        return

    parts = _parse_parts(hotkey)
    if sys.platform == "darwin" and _needs_quartz(parts):
        keycodes = _parts_to_keycodes(parts)
        _trigger_quartz(keycodes, phase=phase, hotkey=hotkey)
        return

    if sys.platform != "darwin":
        parts = [p for p in parts if p not in ("fn", "function")]

    if not parts:
        raise ValueError("hotkey is empty or only contains fn (unsupported on this OS)")

    try:
        import pynput  # noqa: F401
    except ImportError as exc:
        raise RuntimeError("Install bridge deps: tools/run_bridge.sh") from exc

    clean = "+".join(parts)
    modifiers, taps = _parse_pynput_hotkey(clean)
    _trigger_pynput(modifiers, taps, phase=phase, hotkey=clean)


def trigger_enter_key(*, spec: str = "enter", dry_run: bool = False) -> None:
    """
    Simulate Return/Enter on the frontmost app (macOS: System Events osascript).

    spec examples: enter, return, cmd+enter (Cursor approve / Run sometimes needs cmd+enter)
    """
    parts = _parse_parts(spec.replace("return", "enter"))
    if dry_run:
        log.info("[dry-run] would enter spec=%s", spec)
        return

    if sys.platform == "darwin":
        if parts in (["enter"], ["return"]):
            script = 'tell application "System Events" to key code 36'
        elif parts in (["cmd", "enter"], ["command", "enter"]):
            script = 'tell application "System Events" to keystroke return using command down'
        else:
            _quartz_post(0x24, True)
            time.sleep(0.02)
            _quartz_post(0x24, False)
            log.info("action=enter_key | backend=quartz fallback spec=%s", spec)
            return
        try:
            import subprocess

            subprocess.run(["osascript", "-e", script], check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as exc:
            detail = (exc.stderr or exc.stdout or str(exc)).strip()
            raise RuntimeError(f"osascript enter failed: {detail}") from exc
        log.info("action=enter_key | backend=osascript spec=%s", spec)
        return

    try:
        from pynput.keyboard import Controller, Key
    except ImportError as exc:
        raise RuntimeError("Install bridge deps: tools/run_bridge.sh") from exc

    kb = Controller()
    mods = []
    for part in parts[:-1]:
        if part in ("ctrl", "control"):
            mods.append(Key.ctrl)
        elif part in ("cmd", "command"):
            mods.append(Key.cmd)
        elif part in ("shift",):
            mods.append(Key.shift)
        elif part in ("alt", "option"):
            mods.append(Key.alt)
    with kb.pressed(*mods):
        kb.press(Key.enter)
        kb.release(Key.enter)
    log.info("action=enter_key | backend=pynput spec=%s", spec)
