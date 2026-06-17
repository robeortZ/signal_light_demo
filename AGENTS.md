# AGENTS.md — signal_light_demo

AI agent instructions for the `signal_light_demo` TuyaOpen project.

## Project snapshot

- **Name**: `signal_light_demo`
- **Platform**: `t5`
- **Framework**: `base`
- **Source**: Embedded firmware scaffolded from `TuyaOpenSDK/tools/app_template/base` into `source/embedded/`. Build with `cd source/embedded && tos.py build`.

## Project layout

```
signal_light_demo/
├── .tuyaopen/
│   ├── project.json        # name, version, type, framework; ai.intent only when AI-created
│   ├── status.json         # lifecycle: scaffolded → configured → built → flashed
│   ├── architecture.json   # surfaces, modules, components, dependencies
│   ├── dependencies.lock.json   # ecosystem libs (when installed via IDE)
│   ├── ide/                # IDE-managed runtime files (do not edit)
│   │   ├── bin/tuya-devplat-cli   # wrapper script — use full path outside VSCode
│   │   ├── platform.json          # hardware capability spec for the active platform (chip-level)
│   │   └── board.json             # board-level hardware spec (onboard components, pre-allocated pins)
│   └── platform/           # Tuya Platform product snapshots (IDE)
│       └── product-<pid>.json
├── tuyaopen.project.ini    # human-readable project config (IDE-maintained)
├── source/
│   ├── embedded/           # firmware — build with tos.py
│   │   ├── include/        # project header files
│   │   └── src/
│   │       └── tuya_app_main.c
│   └── miniapp/            # panel/mini-app placeholder
├── .vscode/
│   └── settings.json       # CMake source + build directory
├── .agents/skills/         # AI agent skill catalogue (Agent Skills standard)
├── .gitignore
├── AGENTS.md
├── CLAUDE.md
└── README.md
```

## `.tuyaopen/` descriptor contract

**`project.json`**
- `name`, `version`, `type`, `framework` — project identity
- `ai.intent` — user's original intent _(only present for AI-assisted scaffold — absent in normal projects)_
- `ai.assistedScaffold` — true when created via "Create with AI" _(optional — absent in normal projects)_
- `ai.skills` — reserved for future use; **do not rely on it to discover installed skills**. Scan `.agents/skills/` directly — each subdirectory (flat or one level deep) whose `SKILL.md` exists is an installed skill.

**`status.json`** — advance `lifecycle` as the project progresses:
`"scaffolded"` → `"configured"` → `"built"` → `"flashed"`

**`architecture.json`** — update when structure changes:
- `surfaces.embedded` — platform, chip, board, entrypoint, RTOS config, hardware peripherals
- `surfaces.miniapp` — panel kind and entrypoint (null until Panel SDK runs here)
- `modules` — logical modules with source paths and surface
- `dependencies` — external libraries (TuyaOpen core, etc.)

**`dependencies.lock.json`** — created when the IDE installs a library from Library → Ecosystem. Canonical pinned versions; commit to git. The `[ecosystem]` section of `tuyaopen.project.ini` mirrors this file.

**`.tuyaopen/platform/`** — Tuya Platform product snapshots (IDE-owned, read-only). One file per active PID.

- **Binding** — active PID is `tuyaopen.project.ini` → `[product] pid`; the filename is informational.
- **Envelope** — `schemaVersion` (1 or 2), `pid`, `fetchedAt`, `source`, `detail`, `dpSchema`, `fetchError`.
- **Unwrap** — `detail` and `dpSchema` may be wrapped `{ ok, data }`; always read as `field.data ?? field`.
- **Active DPs** — read `dpSchema.dps[]` where `selected === true`; skip all others.
- **Cloud functions** — `dpSchema.uiConfig.bic[]` lists BIC cloud functions enabled for this product.
- **Never invent** — if missing or `fetchError` is set, ask the developer to sync in the IDE.
- **Mutations** — use `tuya-devplat-cli` commands or skill `tuyaopen-dp-add`; never edit the JSON directly.

Deep reference: `.agents/skills/tuya-iot-platform/references/platform-product-snapshot.md`
_(Install skill **Tuya IoT Platform** from the IDE Skills panel if the path does not exist.)_

## `tuyaopen.project.ini`

Human-readable project descriptor at the project root. Maintained by the TuyaOpen IDE; hand-editing is allowed and unknown keys are preserved on round-trips.

| Section | Purpose |
|---|---|
| `[project]` | `name`, `version`, `type`, `schema` — project identity |
| `[platform]` | `target` (`t5` \| `esp32` \| `linux` \| `raspberrypi`), `sdk`, `sdk_version` |
| `[build]` | `output`, `toolchain` |
| `[board]` | Present when a board was selected at creation: `id`, `kconfig_id`, `config_file` (optional fields) |
| `[features]` | Optional feature toggles (`key = value`) |
| `[product]` | Present when bound to a Tuya Platform product: `pid = …` |
| `[ecosystem]` | Present when ecosystem libraries are installed: `owner/name = ^x.y.z` (mirrors `dependencies.lock.json`) |

Prefer updating structured JSON under `.tuyaopen/` for AI-facing contract fields (`project.json`, `architecture.json`). Use the INI when a skill or workflow explicitly targets project config, or when mirroring values the IDE already wrote (product PID, ecosystem constraints).

## Build & flash (embedded)

```bash
cd source/embedded
tos.py check     # verify SDK environment
tos.py build     # compile firmware
tos.py flash     # flash to device
```

> **Skill-script path note**: Skill scripts (e.g. `tuyaopen/dev-loop` `build_run.py`,
> `tuyaopen/debug-helper` `monitor_helper.py`) look for `app_default.config` in CWD
> to locate the project root. Since `app_default.config` lives in `source/embedded/`,
> call these scripts from there and adjust the path accordingly:
>
> ```bash
> cd source/embedded
> $OPEN_SDK_PYTHON ../../.agents/skills/tuyaopen/dev-loop/scripts/build_run.py
> $OPEN_SDK_PYTHON ../../.agents/skills/tuyaopen/debug-helper/scripts/monitor_helper.py start -p /dev/ttyACM1
> ```


## Developer Platform CLI

The TuyaOpen IDE writes a wrapper script at `.tuyaopen/ide/bin/tuya-devplat-cli`.
The IDE injects this directory into the VSCode integrated terminal PATH, but agent
tools running outside VSCode terminals (Claude Code CLI, shell scripts, etc.) must
use the full path explicitly:

```bash
# Prefer the full path so it works in all execution contexts:
.tuyaopen/ide/bin/tuya-devplat-cli auth status --format json
```

The wrapper sets `TUYA_DEVPLAT_CONFIG_PATH` internally, so credentials resolve
correctly regardless of the caller environment.

### Calling the CLI

```bash
.tuyaopen/ide/bin/tuya-devplat-cli auth status --format json          # check login state
.tuyaopen/ide/bin/tuya-devplat-cli product list --format json          # list products
.tuyaopen/ide/bin/tuya-devplat-cli product detail --pid <PID> --format json
.tuyaopen/ide/bin/tuya-devplat-cli product dp-schema --pid <PID> --format json
```

### Write operations — mandatory two-step flow

**Never call `--confirm` without first running `--dry-run`.**

```bash
# Step 1: dry-run (safe, no side effects)
.tuyaopen/ide/bin/tuya-devplat-cli product update --pid <PID> --name "New Name" --dry-run --format json
# → returns { confirmToken, preview, riskLevel }

# Step 2: confirm with the exact same parameters (token is a hash of args)
.tuyaopen/ide/bin/tuya-devplat-cli product update --pid <PID> --name "New Name" --confirm <confirmToken> --format json
```

The CLI validates the confirm token against the command arguments — any modification
of flags between dry-run and confirm will be rejected.

### Credentials

Credentials are stored by the IDE in the directory pointed to by `TUYA_DEVPLAT_CONFIG_PATH`
(set automatically by the wrapper). If the CLI returns an auth error, ask the developer
to sign in via **TuyaOpen IDE → Developer Platform** sidebar.

**Never attempt `.tuyaopen/ide/bin/tuya-devplat-cli auth login` — it requires interactive browser input.**

### Source of truth for product data

Read `.tuyaopen/platform/product-<pid>.json` for cached product/DP data.
Ask the developer to press **Sync** in the IDE if the snapshot is missing or stale.
Do not invent product IDs or DP codes.

## Platform hardware spec (`.tuyaopen/ide/platform.json`)

**Read before writing any hardware-interfacing code.** Authoritative chip-level capability map.

| Field path | What it tells you |
|---|---|
| `peripherals.<name>.tklHeader` | `#include` this header to use the peripheral |
| `peripherals.<name>.idPrefix` | C enum prefix for port/pin IDs (e.g. `TUYA_GPIO_NUM_`, `TUYA_UART_NUM_`) |
| `peripherals.<name>.enableMacro` | Kconfig symbol that gates this peripheral; null = always enabled |
| `peripherals.<name>.count` | Number of available ports / channels |
| `peripherals.<name>.spec` | Port IDs, valid pin groups, parameter ranges (baud, speed, mode, freq…) |
| `peripherals.uart[].ports[*].logPort` | `true` = SDK debug-log port — do **not** reassign |
| `connectivity.<radio>.enabled` | Whether the radio (wifi/ble/ethernet/cellular) exists on this chip |
| `connectivity.<radio>.enableMacro` | Kconfig symbol to enable the radio stack |
| `flashAndDebug.flash.pins` | UART pins used for firmware download — do **not** reassign |
| `flashAndDebug.debug` | Log UART port and baud (read-only; already claimed by the SDK) |
| `pinout[]` | Physical pin ↔ GPIO number ↔ multiplexed functions |
| `memory` | SRAM / ROM / flash / PSRAM sizes in bytes |
| `kconfig.PLATFORM_CHOICE` | Platform Kconfig value for `app_default.config` |

**Workflow**: before using any peripheral, look up `peripherals.<name>` to get the correct header, ID prefix, port IDs, valid pin groups, and parameter constraints. Choose a pin from `spec` that is not already reserved (see `board.json` below and `flashAndDebug.flash.pins`).

## Board hardware spec (`.tuyaopen/ide/board.json`)

**Read before assigning any GPIO.** Chip-agnostic description of what is physically wired on this development board.

| Field | What it tells you |
|---|---|
| `id` / `kconfigId` | Board identifier; pass `kconfigId` to `tos.py config choice -c <kconfigId>` |
| `platformId` | Matches `platform.json.id` — links the two files |
| `peripheralPatterns.<type>[]` | Onboard components (LED, button, audio, display, …) |
| `peripheralPatterns.<type>[].pins.<if>[].gpio` | GPIO number already occupied by this component |
| `peripheralPatterns.<type>[].note` | Polarity / behaviour (active-high, active-low, AEC enabled…) |
| `peripheralPatterns.<type>[].kconfig` | Kconfig symbol to enable this component (null = always active) |
| `scaffold.baseConfig` | Kconfig lines required in `app_default.config` for this board |

**Workflow**: before wiring a new GPIO, collect all reserved numbers —
`peripheralPatterns.<type>[].pins.<if>[].gpio` across every entry — then pick a free one from `platform.json peripherals.gpio.spec.pins`. Never repurpose a reserved GPIO without developer approval; these are physical PCB traces.

## Conventions

- Entry point: `tuya_app_main()` in `source/embedded/src/tuya_app_main.c`
- Use `PR_DEBUG(fmt, ...)` for debug output (wraps `tal_log`)
- Use TAL APIs (`tal_system_sleep`, `tal_thread_create_and_start`, etc.) — do not call platform-specific functions directly
- Run `tos.py check` after environment changes before building
- Update `status.json.lifecycle` as the project progresses
- Surface open questions to the developer; do not invent requirements

## Coding Style

Follow POSIX C style — use standard lowercase keywords:

```c
static void   app_led_init(void);       // correct
static int    count = 0;                // correct
const uint8_t *buf;                     // correct

STATIC VOID   app_led_init(VOID);      // WRONG — never use uppercase keyword aliases
```

- C keywords (`static`, `void`, `const`, `int`, `return`, …) are always lowercase
- SDK typedefs that are uppercase are fine: `OPERATE_RET`, `TDL_LED_HANDLE_T`, `TUYA_GPIO_NUM_E`, etc.
- Never use `STATIC`, `VOID`, `CONST`, `INT` even if the SDK headers define them
