---
name: Wire ecosystem library into CMake
description: Wire a downloaded PlatformIO-Registry library into the active TuyaOpen project's CMakeLists.txt and (where applicable) Kconfig / tos.py module list. The pinned version, source URL, and install path are already declared in `.tuyaopen/dependencies.lock.json` (and mirrored into the `[ecosystem]` section of `tuyaopen.project.ini`) — read those files first instead of guessing.
when_to_use: Triggered automatically after the IDE downloads a third-party library from the Library → Ecosystem tab into `source/embedded/dependencies/<owner>/<name>/`. Use it when the user asks the agent to "wire", "integrate", or "register" a freshly-installed library into the build.

id: ecosystem-wire-cmake
surfaces: [embedded]
tags: [cmake, dependencies, vibeCode]
command: tuyaopen.skill.wireCmake
fallback_commands: []
default_enabled: true
related: [tuyaopen-build, tuyaopen-project-config]
i18n_title: skill.wireCmake.title
i18n_description: skill.wireCmake.description
i18n_when: skill.wireCmake.when
---

# Wire ecosystem library into CMake

Wire a downloaded PlatformIO-Registry library into the active TuyaOpen project's CMakeLists.txt and (where applicable) Kconfig / tos.py module list. The pinned version, source URL, and install path are already declared in `.tuyaopen/dependencies.lock.json` (and mirrored into the `[ecosystem]` section of `tuyaopen.project.ini`) — read those files first instead of guessing.

## When to use

Triggered automatically after the IDE downloads a third-party library from the Library → Ecosystem tab into `source/embedded/dependencies/<owner>/<name>/`. Use it when the user asks the agent to "wire", "integrate", or "register" a freshly-installed library into the build.

## Prerequisites

- A TuyaOpen project is open as the workspace root.
- The library has been extracted into `<project>/source/embedded/dependencies/<owner>/<name>/` (the IDE has already done this — its on-disk inventory is in `dependencies/manifest.json` and the project-level declaration is in `.tuyaopen/dependencies.lock.json`).
- You have read/write access to the project's `CMakeLists.txt` files.

## How the agent should invoke it

Prefer the **Run Command** tool with the registered VSCode command id:

```
Run Command "tuyaopen.skill.wireCmake"
```

If `Run Command` is unavailable, fall back to the SDK terminal:

```bash
# from the project root, with the TuyaOpen SDK env active
cd <project> && # (no direct tos.py command — edit CMakeLists.txt and re-run `tos.py build`)
```

## Arguments

- `owner` — PlatformIO library owner (folder name).
- `name` — PlatformIO library name (folder name).
- `installedPath` — absolute path to the extracted library root.
- `installedVersion` — exact pinned version (read from `.tuyaopen/dependencies.lock.json`).

## Success signal

The project's main `CMakeLists.txt` references the new directory via `add_subdirectory()` *or* an inline `target_include_directories()` + `target_sources()` block, the project still builds via `tos.py build`, and the change has been described back to the user. The lockfile and `[ecosystem]` section of `tuyaopen.project.ini` are *not* edited — they are owned by the IDE.

## Related skills

- `tuyaopen-build`
- `tuyaopen-project-config`

_Maintained in the TuyaOpen IDE skills registry. Reinstall the skill from the IDE's Skills page after registry updates._
