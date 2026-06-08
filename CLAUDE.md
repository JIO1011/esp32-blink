# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project context

This is the firmware repository for the **Sistema de Alarmas Comunitarias y Botón de Pánico** (COMSECA / Ambato, ANEXO 1). Target hardware: **ESP32 DevKit V1 (WROOM-32)** + SIM800L + DFPlayer Mini (TF-16P) + MicroSD module.

Two project skills are installed in `.claude/skills/`:

- **`alarmas-comunitarias-hw/`** — authoritative reference for hardware constraints, pinout, ANEXO 1 compliance checklist, and demo-vs-production decisions. Consult before any firmware or hardware change. Contains `references/bases-anexo1.md` and `references/at-sim800l.md`.
- **`firmware-esp32-platformio/`** — firmware architecture guide: project structure under `lib/`, FreeRTOS task layout, state machine pattern, ISR conventions, dependency injection, error handling, and testing with `env:native`. Two-tier criteria: **Siempre** (always: ISR safety, mutex, no blocking delay, Config.h) and **Por defecto** (preferred defaults: SRP, DI, interfaces, logger, tests — deviate with explicit justification). Contains `references/patrones-firmware.md` and `references/platformio-setup.md`.

## Project documents

- **[instrucciones.md](instrucciones.md)** — 10-phase firmware roadmap (Fase 0–9), each with objective, ANEXO 1 requirements covered (with source), firmware work, hardware needed, and done criteria. Phases 0–6 cover the minimum demo; 7–9 complete ANEXO 1 compliance on the ESP32 side.
- **[requerimientos-modulos.md](requerimientos-modulos.md)** — Bill of materials for missing modules. Bloque A (demo) + B1 (LAN W5500), plus Producción rows (P1–P5) for items outside the prototype scope (homologated GSM, AC supply, power stage, enclosure, RTC).

## Toolchain

PlatformIO CLI (`pio`) — target board `esp32doit-devkit-v1`, framework Arduino, C++17.

```bash
pio run                          # compile
pio run -t upload                # compile + flash (port /dev/ttyUSB0)
pio device monitor               # serial monitor at 115200 baud
pio run -t upload && pio device monitor   # flash then monitor
pio run -t clean                 # clean build artifacts
pio test -e native               # run logic tests on PC (no ESP32 needed)
```

Add libraries to `platformio.ini` under `lib_deps`; PlatformIO resolves and caches them automatically. Never commit `.pio/`. Target `platformio.ini` structure (from `firmware-esp32-platformio/references/platformio-setup.md`):

```ini
[env:esp32dev]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
build_flags = -std=gnu++17
build_unflags = -std=gnu++11
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time

[env:native]
platform = native
test_framework = unity
build_flags = -std=gnu++17
```

## Architecture

`src/main.cpp` is the single entry point — wires objects, injects dependencies, launches FreeRTOS tasks. All logic lives in `lib/` components behind interfaces. Current state: blink sketch; target structure starts at **Fase 0** of `instrucciones.md`.

Target `lib/` split:

| Component | Responsibility | Interface |
|-----------|---------------|-----------|
| `Hal` | Thin Arduino wrapper (GPIO, UART, SPI) | — |
| `Sim800Driver` | AT commands, SMS reception, GPRS/HTTP | `IGsm` |
| `Mp3Player` | DFPlayer audio playback by event type | `IAudioPlayer` |
| `SdStorage` | MicroSD event log and config (VSPI) | `IStorage` |
| `EventLogger` | Leveled logging (info/warn/error) | — |
| `AlarmController` | State machine, orchestrates all modules | depends on interfaces only |

All pin assignments and baudrates live in `include/Config.h` (`namespace Config`, `constexpr`).

### Peripheral pinout

| Peripheral | ESP32 pins | Notes |
|------------|------------|-------|
| SIM800L RX ← SIM TX | GPIO26 | direct |
| SIM800L TX → SIM RX | GPIO27 | voltage divider to ~2.8 V |
| DFPlayer RX ← DF TX | GPIO16 | |
| DFPlayer TX → DF RX | GPIO17 | 1 kΩ series |
| MicroSD VSPI | SCK=18, MISO=19, MOSI=23, CS=5 | FAT32, event log |
| LAN W5500 (Fase 8) | shared SPI, CS=GPIO4 | third connectivity mode |
| Relay placeholder | GPIO13 | future power stage |
| Status LED | GPIO2 | heartbeat / state |

### Hardware constraints (non-negotiable)
- SIM800L requires a **dedicated 3.4–4.4 V rail at ≥ 2 A** (buck MP1584 ~4.0 V) + 1500 µF bulk cap — never power from ESP32 3V3.
- GPIO6–11 reserved for internal flash.
- Strapping pins (0, 2, 12, 15) and input-only pins (34–39) must not be used as peripheral TX.
- On WROVER modules GPIO16/17 are occupied by PSRAM — reassign DFPlayer to 32/33 and SIM800L to 25/26.
