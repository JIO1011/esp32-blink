# Guía rápida de módulos

Referencia de uso por módulo: qué hace, cómo se controla, qué archivos toca y **dónde editar para agregar/quitar**. Detalle de hardware → skill `alarmas-comunitarias-hw`; arquitectura → skill `firmware-esp32-platformio`.

## Comandos generales
```bash
~/.platformio/penv/bin/pio run -e esp32dev            # compilar       (pio no está en PATH)
~/.platformio/penv/bin/pio run -e esp32dev -t upload  # flashear (placa pelada si falla; ver nota brownout)
~/.platformio/penv/bin/pio device monitor -e esp32dev # monitor serial 115200
~/.platformio/penv/bin/pio test -e native             # tests de lógica en la PC
```
**Consola serial (demo, sin hardware extra):** `p`=pánico · `m`=simula SMS · `r`=simula REST · `s`=silenciar · `a`=ack.
**Config central:** todos los pines/baudrates/constantes en [include/Config.h](include/Config.h).

---

## Módulo MP3 — DFPlayer Mini (audio por tipo de evento, Fase 2)
- **HW:** VCC→**5V**, GND común; ESP32 GPIO16←DF_TX, GPIO17→[1 kΩ]→DF_RX; parlante en SPK_1/SPK_2; su **propia** microSD FAT32 con `/mp3/0001.mp3`, `0002.mp3`…
- **Uso:** en cada disparo el controlador llama `audio.play(pista)`; pista según el evento (`p`→1 pánico, `m`→2 sospechoso, `r`→3 disuasivo). `s` detiene.
- **Archivos:** [lib/Mp3Player/](lib/Mp3Player/), [lib/Common/IAudioPlayer.h](lib/Common/IAudioPlayer.h), `Config.h` (`PIN_MP3_*`, `MP3_BAUD`, `TRACK_*`), mapeo en [AlarmController.cpp](lib/AlarmController/AlarmController.cpp) → `trackFor()`.
- **Agregar/quitar pista:** copia `000N.mp3` a la SD + define `TRACK_X` en `Config.h` + mapea en `trackFor()`. **Volumen:** arg `volume` (0–30) en `Mp3Player` (def. 25).

---

## Módulo MicroSD del ESP32 — log de eventos (Fase 3)
- **HW:** módulo VSPI SCK=18 / MISO=19 / MOSI=23 / CS=5; VCC según módulo (mini pelado→3V3, azul con chips→5V); GND común; FAT32; **tarjeta distinta** de la del DFPlayer.
- **Uso:** cada transición escribe una línea `timestamp_ms,origen,estado` en `/eventos.csv`. Al arrancar se vuelca el log por Serial (continuidad tras reinicio).
- **Archivos:** [lib/SdStorage/](lib/SdStorage/), [lib/Common/IStorage.h](lib/Common/IStorage.h), `Config.h` (`PIN_SD_*`, `LOG_EVENTS_PATH`).
- **Editar:** ruta/nombre → `LOG_EVENTS_PATH`; formato de línea → `SdStorage::logEvent()`; pines → `PIN_SD_*`. La SD se monta **solo al arrancar** (si falla, reinicia tras reconectar).

---

## Módulo SIM800L — activación por SMS (Fase 4)
- **HW (crítico):** rail dedicado **~4 V ≥2 A + cap 1500 µF** (NUNCA del 3V3); SIM_TX→GPIO26, GPIO27→[divisor 1k+2.2k≈2.8 V]→SIM_RX; antena GSM; SIM 2G activa; GND común.
- **Uso:** un SMS con `PANICO/EMERGENCIA/AUXILIO/ALARMA/SOS` **dispara**; `SILENCIAR/APAGAR/OK` **silencia**; sólo si el remitente está en la allow-list. `TaskGsm` valida y publica el evento en la cola.
- **Archivos:** [lib/Sim800Driver/](lib/Sim800Driver/), [lib/Common/IGsm.h](lib/Common/IGsm.h), [lib/Common/SmsCommand.h](lib/Common/SmsCommand.h) (lógica pura testeada), `Config.h` (`PIN_SIM_*`, `GSM_BAUD`, `SMS_ALLOWLIST`).
- **Editar:** número autorizado → por el **portal** (NVS) o `SMS_ALLOWLIST`; palabras clave → `SmsCommand::toEvent()`; secuencia AT → `Sim800Driver::initialize()`.

---

## WiFi + Portal de configuración (Fase 5)
- **HW:** ninguno (WiFi integrado). Modo **Station + SoftAP** simultáneo.
- **Uso:** conéctate al SoftAP `AlarmaComunitaria-Config` (clave `alarma1234`), abre `http://192.168.4.1/` y configura SSID/clave WiFi y la allow-list de SMS. Se guarda en **NVS** y persiste tras reinicio (credenciales fuera del código/git).
- **Archivos:** [lib/NetPortal/](lib/NetPortal/), [lib/ConfigStore/](lib/ConfigStore/), `Config.h` (`AP_SSID`, `AP_PASS`).
- **Editar:** SSID/clave del portal → `AP_SSID`/`AP_PASS`; campos del formulario → `NetPortal::handleRoot/handleSave`; claves guardadas → `ConfigStore`.
