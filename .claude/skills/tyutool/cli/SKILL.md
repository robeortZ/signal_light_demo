---
name: tyutool/cli
description: >-
  Direct tyutool_cli usage for TuyaOpen hardware: list serial ports,
  flash firmware (write), read flash, hardware reset (DTR/RTS), and
  authorize devices (UUID/AuthKey via UART). Use when the user mentions
  tyutool, flashing directly, reading flash, hardware reset, or UART
  authorization. Tool at $OPEN_SDK_ROOT/tools/tyutool/tyutool_cli
  (Linux/macOS) or tyutool_cli.exe (Windows). Install: tos.py update -t.
  固件烧录、Flash读取、硬件复位、串口设备授权（UUID/AuthKey）。
license: Apache-2.0
compatibility:
  - TuyaOpen environment activated (export.sh / export.ps1 / export.bat)
  - tyutool_cli installed at $OPEN_SDK_ROOT/tools/tyutool/ (tos.py update -t)
  - Device connected via USB serial
---

# tyutool CLI

> **SDK root:** All tool paths use `$OPEN_SDK_ROOT` (Linux/macOS/PowerShell) or `%OPEN_SDK_ROOT%` (Windows CMD).

## Tool Location & Install

| Platform | Binary |
|----------|--------|
| Linux / macOS | `$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli` |
| Windows PowerShell | `$env:OPEN_SDK_ROOT\tools\tyutool\tyutool_cli.exe` |
| Windows CMD | `%OPEN_SDK_ROOT%\tools\tyutool\tyutool_cli.exe` |

```bash
tos.py update -t   # install or force-update (--tyutool flag)
tos.py update      # also updates tyutool if already installed

$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --version   # verify
```

## Global Flag: `--plain`

Always add `--plain` — disables ANSI/spinner, produces stable parseable output for agents.

## Chip Parameters

Three baud rates per chip:
- **Flash baud**: used by `write`/`read`. Recommended rate; higher = faster but risk instability at physical limits. Do not override unless developer instructs; if flash fails with instability, ask developer whether to lower baud.
- **Auth baud**: used by `authorize` (default 115200, all chips). Developer may change firmware UART baud — if garbled output during auth, ask developer for correct rate.
- **Log baud**: used for serial monitor. Developer may change — if garbled monitor output, ask developer for correct rate.

| `--device` | Flash baud | Auth baud | Log baud | Flash size |
|------------|-----------|----------|---------|-----------|
| `bk7231n` | 921600 | 115200 | 115200 | 2 MiB (`0x200000`) |
| `t2` | 921600 | 115200 | 115200 | 2 MiB (`0x200000`) |
| `t3` | 921600 | 115200 | 460800 | 4 MiB (`0x400000`) |
| `t1` | 921600 | 115200 | 115200 | 8 MiB (`0x800000`) |
| `t5` | 921600 | 115200 | 460800 | 8 MiB (`0x800000`) |
| `ln882h` | 115200 | 115200 | 115200 | 2 MiB (`0x200000`) |
| `esp32` | 460800 | 115200 | 115200 | 4 MiB (`0x400000`) |
| `esp32c3` | 460800 | 115200 | 115200 | 4 MiB (`0x400000`) |
| `esp32c6` | 460800 | 115200 | 115200 | 8 MiB (`0x800000`) |
| `esp32s3` | 460800 | 115200 | 115200 | 16 MiB (`0x1000000`) |

## Port Selection (Do First)

```bash
$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --plain list-ports
```

Output (tab-separated): `path  vid:pid  usb_interface  port_role  display_name`

**T5/T5AI dual-serial boards** (VID `1a86` PID `55d2`): two ports per device.
- **Flash port** (lower enumeration: `ttyACM0`, lower COM) → `write`, `reset`, `authorize`
- **Log port** (higher enumeration: `ttyACM1`, higher COM) → serial monitor / log capture

This mapping is typical but not guaranteed — swap ports if flash fails.

Always pass `-p <port>` explicitly. Omitting `-p` with multiple ports triggers an interactive selection prompt (not usable in agent workflows).

## `write` — Flash Firmware

```bash
$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --plain write \
    -d <chip> -f <firmware.bin> -p <flash_port> \
    [-b <baud>] [-s <start_hex>] [--end <end_hex>]
```

| Flag | Required | Default |
|------|----------|---------|
| `-d/--device` | YES | — |
| `-f/--file` | YES | — |
| `-p/--port` | NO* | auto-detect |
| `-b/--baud` | NO | chip flash baud |
| `-s/--start` | NO | `0x00000000` |
| `--end` | NO | `start + file size` |

*Always specify `-p` for agents.

Success: exit 0, last output line is `Flash OK  Xs`.

## `read` — Dump Flash to File

```bash
$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --plain read \
    -d <chip> -f <output.bin> -p <flash_port> \
    [-s <start_hex>] [-l <length_hex>]
```

Default length: `0x200000` (2 MiB). For a full dump, use the chip flash size from the table above.

## `reset` — Hardware Reset via DTR/RTS

Use the **flash port** (same as `write`).

```bash
$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --plain reset \
    -d <chip> -p <flash_port>
```

`-d` default: `bk7231n`. Reset sequence by family:
- Beken (`bk7231n`, `t1`–`t5`): DTR/RTS pulse
- ESP32 (`esp32`, `esp32c3`, `esp32c6`, `esp32s3`): espflash hard_reset

## `authorize` — Read/Write UUID + AuthKey

Use the **flash port**. Operates at the chip's auth baud (115200 by default).

```bash
# Read current auth state (omit --uuid and --authkey)
$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --plain authorize -p <flash_port>

# Write credentials (both flags required together)
$OPEN_SDK_ROOT/tools/tyutool/tyutool_cli --plain authorize \
    -p <flash_port> --uuid <uuid_string> --authkey <authkey_string>
```

Obtain UUID/AuthKey from the Tuya platform — see skill `tuyaopen/device-auth`.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `tyutool_cli: not found` | `tos.py update -t` |
| `No serial ports found` | Re-plug USB cable |
| Handshake fails / timeout | Try the other port |
| Flash unstable / fails | Ask developer whether to use lower baud (`-b 460800`) |
| Garbled output during `authorize` | Ask developer for firmware's UART auth baud rate |
| Garbled serial monitor output | Ask developer for firmware's UART log baud rate |
| Interactive port prompt appears | Always pass `-p <port>` explicitly |

## Diagnostic Logs

On failure, check the tool's own log file:
- Linux: `~/.local/share/tyutool/tyutool.log`
- macOS: `~/Library/Application Support/tyutool/tyutool.log`
- Windows: `%APPDATA%\tyutool\tyutool.log`
