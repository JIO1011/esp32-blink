# 🛠️ Guía rápida de módulos

Referencia de uso y **conexiones** por módulo. HW detallado → skill `alarmas-comunitarias-hw` · arquitectura → `firmware-esp32-platformio`.

---

## ⚡ Pinout maestro (ESP32 DevKit V1)

| GPIO | Conectar a | Dirección / nota | Módulo |
|:---:|:---|:---|:---|
| **2**  | LED de placa | salida (latido/fijo) | Estado |
| **13** | LED+220Ω o IN de relé | salida de disparo (placeholder potencia) | Disparo |
| **25** | Pulsador → GND | entrada, pull-up interno | Botón pánico |
| **16** | DFPlayer **TX (3)** | ← entra al ESP32 | MP3 |
| **17** | DFPlayer **RX (2)** | → sale del ESP32, **1 kΩ en serie** | MP3 |
| **18 / 19 / 23 / 5** | MicroSD SCK / MISO / MOSI / CS | bus VSPI | MicroSD |
| **26** | SIM800L **TX** | ← entra al ESP32 (directo) | SIM800L |
| **27** | SIM800L **RX** | → sale del ESP32, **divisor 1k+2.2k (~2.8 V)** | SIM800L |
| **33** | SIM800L RST | salida (opcional) | SIM800L |
| 4 | LAN W5500 CS | VSPI compartido (Fase 8, futuro) | LAN |

> ⚠️ **Alimentación (regla de oro):** **GND común** entre todo.
> · **DFPlayer → 5V** · **MicroSD → 3V3** (mini pelado) o **5V** (módulo con chips) · **ESP32 → USB**
> · **SIM800L → rail dedicado ~4 V a ≥2 A + cap 1500 µF. NUNCA del 3V3** (causa #1 de fallos).

---

## 🖥️ Comandos (PlatformIO — `pio` no está en PATH)
```bash
~/.platformio/penv/bin/pio run -e esp32dev             # compilar
~/.platformio/penv/bin/pio run -e esp32dev -t upload   # flashear (placa pelada si el upload falla)
~/.platformio/penv/bin/pio device monitor -e esp32dev  # monitor serial 115200
~/.platformio/penv/bin/pio test -e native              # tests de lógica en la PC
```
## ⌨️ Consola serial (demo, sin hardware extra)
`p` pánico · `m` simula SMS · `r` simula REST · `s` silenciar · `a` ack

---

## 🔧 Recetas — ¿dónde cambio…?

| Quiero… | Archivo / lugar | Qué editar |
|:---|:---|:---|
| **Números autorizados (lista blanca SMS)** | **Portal** (recomendado) **o** [include/Config.h](include/Config.h) | En `http://192.168.4.1/` campo *Numeros…* (persiste en NVS). En código: `SMS_ALLOWLIST` (separados por coma) |
| **Abrir el portal `192.168.4.1`** | _(teléfono/PC)_ | Conéctate al WiFi **`AlarmaComunitaria-Config`** (clave `alarma1234`) → navegador → `http://192.168.4.1/` |
| **Clave/nombre del SoftAP de config** | [include/Config.h](include/Config.h) | `AP_PASS` (≥8 chars) y `AP_SSID` |
| **Conectar a tu WiFi de casa** | **Portal** | en `192.168.4.1`, SSID + clave (persiste, no se recompila) |
| **Letras de la consola (`p`,`s`,`m`,`r`,`a`)** | [src/main.cpp](src/main.cpp) | función `TaskSerialConsole` → el `switch (c)` |
| **Agregar/cambiar un audio** | SD del DFPlayer + [include/Config.h](include/Config.h) + [AlarmController.cpp](lib/AlarmController/AlarmController.cpp) | copia `/mp3/000N.mp3` + define `TRACK_X` + mapea en `trackFor()` |
| **Volumen del audio** | [include/Config.h](include/Config.h) | `MP3_VOLUME` (0–30) |
| **Palabras clave del SMS** (PANICO, SILENCIAR…) | [lib/Common/SmsCommand.cpp](lib/Common/SmsCommand.cpp) | función `toEvent()` |
| **Webhook / APN** (notificación Fase 7) | [include/Config.h](include/Config.h) | `WEBHOOK_URL`, `GSM_APN` |
| **Ruta del archivo de log** | [include/Config.h](include/Config.h) | `LOG_EVENTS_PATH` |
| **Cualquier pin** | [include/Config.h](include/Config.h) | `PIN_*` |

> 🧭 Regla: **casi todo se cambia en [include/Config.h](include/Config.h)** (fuente única). Lo que cambia en caliente (WiFi, lista blanca) va por el **portal**.

---

## 🔊 MP3 — DFPlayer Mini · _audio por tipo de evento (Fase 2)_

🔌 **Conexiones**

| ESP32 | ↔ | DFPlayer | Nota |
|:---:|:---:|:---|:---|
| 5V | → | VCC (1) | mejor a 5 V |
| GPIO16 | ← | TX (3) | directo |
| GPIO17 | → | RX (2) | **1 kΩ en serie** |
| GND | — | GND (7,10) | común |
| — | | SPK_1 (6) / SPK_2 (8) | parlante 4–8 Ω |

▶️ **Uso:** disparo → `audio.play(pista)`; `p`→1 pánico · `m`→2 sospechoso · `r`→3 disuasivo · `s` para. SD propia FAT32 con `/mp3/0001.mp3`…
📁 **Archivos:** [lib/Mp3Player/](lib/Mp3Player/) · [IAudioPlayer.h](lib/Common/IAudioPlayer.h) · `Config.h` (`PIN_MP3_*`,`MP3_BAUD`,`TRACK_*`) · `trackFor()` en [AlarmController.cpp](lib/AlarmController/AlarmController.cpp)
✏️ **Agregar/quitar pista:** copia `000N.mp3` a la SD + `TRACK_X` en `Config.h` + mapea en `trackFor()`. Volumen (0–30): arg de `Mp3Player`.

---

## 💾 MicroSD del ESP32 · _log de eventos (Fase 3)_

🔌 **Conexiones (bus VSPI)**

| ESP32 | ↔ | MicroSD |
|:---:|:---:|:---|
| GPIO18 | → | SCK / CLK |
| GPIO19 | ← | MISO / DO |
| GPIO23 | → | MOSI / DI |
| GPIO5  | → | CS / SS |
| 3V3 o 5V | → | VCC (según módulo) |
| GND | — | GND |

▶️ **Uso:** cada transición escribe `timestamp,origen,estado` en `/eventos.csv`; se vuelca al arrancar. Tarjeta **distinta** de la del DFPlayer.
📁 **Archivos:** [lib/SdStorage/](lib/SdStorage/) · [IStorage.h](lib/Common/IStorage.h) · `Config.h` (`PIN_SD_*`,`LOG_EVENTS_PATH`)
✏️ **Editar:** ruta → `LOG_EVENTS_PATH` · formato → `SdStorage::logEvent()`. Se monta **solo al arrancar** (si falla, reconecta y **reinicia**).

---

## 📶 SIM800L · _activación por SMS (Fase 4)_

🔌 **Conexiones**

| ESP32 | ↔ | SIM800L | Nota |
|:---:|:---:|:---|:---|
| GPIO26 | ← | TX | directo |
| GPIO27 | → | RX | **divisor 1k+2.2k (~2.8 V)** |
| GPIO33 | → | RST | opcional |
| **rail 4 V** | → | VCC | **dedicado ≥2 A + cap 1500 µF** |
| GND | — | GND | común con el rail y el ESP32 |
| — | | antena uFL + SIM 2G | sin antena no registra |

▶️ **Uso:** SMS `PANICO/EMERGENCIA/AUXILIO/ALARMA/SOS` → dispara · `SILENCIAR/APAGAR/OK` → silencia · solo remitentes de la allow-list.
📁 **Archivos:** [lib/Sim800Driver/](lib/Sim800Driver/) · [IGsm.h](lib/Common/IGsm.h) · [SmsCommand.h](lib/Common/SmsCommand.h) · `Config.h` (`PIN_SIM_*`,`GSM_BAUD`,`SMS_ALLOWLIST`)
✏️ **Editar:** número autorizado → **portal web** (NVS) o `SMS_ALLOWLIST` · palabras clave → `SmsCommand::toEvent()` · AT → `Sim800Driver::initialize()`

---

## 🌐 WiFi + Portal (Fase 5) + REST API (Fase 6) · _sin hardware extra_

🔌 **Conexiones:** ninguna (WiFi integrado). Modo **Station + SoftAP** simultáneo.

▶️ **Config inalámbrica:** conéctate al SoftAP **`AlarmaComunitaria-Config`** (clave `alarma1234`) → abre **`http://192.168.4.1/`** → SSID/clave WiFi + allow-list. Persiste en NVS.

▶️ **API REST (curl):**
```bash
curl http://<ip>/api/estado                      # {"estado":"ARMED","red":"WiFi-STA","rssi":-58,"ip":"..."}
curl -X POST http://<ip>/api/alarma?tipo=panico   # dispara (tipo: panico|sospechoso|disuasivo)
curl -X POST http://<ip>/api/silenciar            # silencia
curl http://<ip>/api/eventos                      # log CSV de la MicroSD
```
🔔 **Notificación saliente (Fase 7):** en cada evento, el equipo hace un **POST JSON** a `WEBHOOK_URL` por **WiFi** si hay (testeable ya) o por **GPRS** (SIM800L) si solo hay 2G.
📁 **Archivos:** [lib/NetPortal/](lib/NetPortal/) · [lib/ConfigStore/](lib/ConfigStore/) · [lib/RestApi/](lib/RestApi/) · `TaskGsm` en [src/main.cpp](src/main.cpp) (POST) · `Config.h` (`AP_*`,`WEBHOOK_URL`,`GSM_APN`)
✏️ **Editar:** SSID/clave del portal → `AP_SSID`/`AP_PASS` · campos del formulario → `NetPortal::handleRoot/handleSave` · endpoints → `RestApi::registerRoutes()` · webhook → `WEBHOOK_URL`
