# Instrucciones — Firmware ESP32 · Alarmas Comunitarias (COMSECA / Ambato)

Hoja de ruta **acotada a la ESP32**: firmware y los módulos que se conectan directamente a ella
(SIM800L, DFPlayer Mini, MicroSD, WiFi/SoftAP, REST, LAN W5500). Quedan **fuera de este documento**
porque no dependen del microcontrolador: app móvil/PWA, backend/servidores/AWS, gabinete IP65,
fuente 120–240 VAC, batería 12 V 7 A, amplificador de 200 W, sirena y estrobo (etapa de potencia de
producción). El firmware solo deja **un GPIO de disparo** preparado para esa etapa.

Referencias autoritativas:
- Hardware y cumplimiento ANEXO 1 → skill `alarmas-comunitarias-hw`
- Arquitectura del código → skill `firmware-esp32-platformio`

Cada fase indica: **objetivo**, **requisitos que cubre (con su fuente)**, **trabajo de firmware**,
**hardware que requiere** y **criterio de "hecho"**. Las fases son incrementales: cada una deja el
sistema compilando y demostrable.

**Trazabilidad de fuentes** (de dónde sale cada requisito):
- `bases §N` → sección N de `alarmas-comunitarias-hw/references/bases-anexo1.md` (extracto del pliego ANEXO 1).
- `hw-skill` → reglas de hardware/pinout en `alarmas-comunitarias-hw/SKILL.md`.
- `at-sim800l` → `alarmas-comunitarias-hw/references/at-sim800l.md`.
- `fw-skill` → arquitectura de código en `firmware-esp32-platformio/SKILL.md`.

---

## Estado actual
- `src/main.cpp` es un *blink* + `Serial.println` de prueba.
- `platformio.ini`: `env:esp32dev`, board `esp32dev`, framework arduino, **C++11**.
- No hay estructura `lib/`, ni `Config.h`, ni tests.

Punto de partida real: **Fase 0**.

---

## Fase 0 — Cimientos del firmware (sin hardware nuevo)
**Objetivo:** dejar el esqueleto arquitectónico antes de tocar periféricos.
**Requisitos que cubre (fuente):** ninguno visible aún; habilita todo lo demás. La estructura `lib/`,
interfaces, `Config.h`, máquina de estados y testeo en `env:native` salen de `fw-skill` (estructura del
proyecto + criterios "Siempre"/"Por defecto").

**Firmware**
- Migrar `platformio.ini`: board `esp32doit-devkit-v1`, `build_flags = -std=gnu++17`, `build_unflags = -std=gnu++11`, `monitor_filters = esp32_exception_decoder, time`. Añadir `[env:native]` con `test_framework = unity`.
- Crear estructura `lib/` (Hal, Common, EventLogger, AlarmController) e `include/Config.h` con pines y baudrates del pinout de la skill de hardware.
- Implementar `ErrorCode`, `Logger` (niveles info/warn/error), `enum class AlarmState` y `EventType`.
- `main.cpp` reducido a composición: crear objetos, inyectar, lanzar `TaskController` y la `eventQueue`.

**Hardware:** solo la ESP32 (ya disponible).
**Hecho cuando:** compila en `esp32dev` y `native`; `pio test -e native` corre (aunque sea 1 test trivial); el `TaskController` arranca y registra su estado por el logger.

---

## Fase 1 — Botón de pánico por ISR + máquina de estados
**Objetivo:** validar el patrón evento→cola→controlador con la fuente más simple.
**Requisitos que cubre (fuente):** activación por **pulsador físico** (`bases §7` botón de emergencia:
"activación por botón físico"; `bases §4` pulsantes principales). Patrón ISR→cola y máquina de estados
explícita → `fw-skill`.

**Firmware**
- ISR del botón (`IRAM_ATTR`) → `xQueueSendFromISR(eventQueue, EventType::BTN_PANIC)`.
- Antirrebote por software (marca de tiempo en la tarea, no en la ISR).
- Transiciones `IDLE/ARMED → TRIGGERED → SILENCED` en `AlarmController::onEvent` (switch).
- LED de estado (GPIO2): latido en ARMED, fijo en TRIGGERED.
- GPIO de disparo (GPIO13) como *placeholder* de la futura etapa de potencia.
- Test en `env:native`: transiciones de la máquina de estados con eventos simulados.

**Hardware:** ESP32 + 1 pulsador + LED de placa. **Sin compras.**
**Hecho cuando:** pulsar → estado pasa a TRIGGERED y se registra; un evento SILENCE lo silencia; tests de estado en verde.

---

## Fase 2 — Audio por tipo de evento (DFPlayer Mini)
**Objetivo:** reproducir el audio correcto según el evento.
**Requisitos que cubre (fuente):** "audio configurado **por tipo de evento**, mínimo pánico, sospechoso,
pregrabados, disuasivo" y "≥2 audios automáticos" (`bases §1`). Convenciones del DFPlayer (UART 9600,
`/mp3/000x.mp3`, 1 kΩ en RX) → `hw-skill`.

**Firmware**
- Interfaz `IAudioPlayer` + driver `Mp3Player` (UART1, 9600 bps, RX=16 / TX=17 con 1 kΩ en serie).
- Mapa evento→pista en `Config.h` (`TRACK_PANIC=1`, `TRACK_SUSPICIOUS=2`, `TRACK_DETERRENT=3`…).
- El controlador llama `audio.play(trackFor(ev))` en la transición a TRIGGERED y `audio.stop()` al silenciar.
- Esperar ~1–2 s tras `begin()` antes del primer `play`; manejar `ErrorCode::AUDIO_ERROR`.

**Hardware requerido (compra):** parlante 4–8 Ω ≤3 W, microSD para el DFPlayer (FAT32, `/mp3/0001.mp3`…), resistencia 1 kΩ.
**Hecho cuando:** cada tipo de evento dispara su pista; sin clics ni reinicios de audio.

---

## Fase 3 — Log de eventos en MicroSD (lado ESP32)
**Objetivo:** auditoría/respaldo local de cada evento.
**Requisitos que cubre (fuente):** base local del "backup automático de BD" y los respaldos del sistema
(`bases §4` y `§6`) — la parte que vive en el dispositivo. Pinout VSPI (SCK=18/MISO=19/MOSI=23/CS=5) y
SD independiente de la del DFPlayer → `hw-skill`. Mutex sobre la SD → `fw-skill` (criterio "Siempre").

**Firmware**
- Interfaz `IStorage` + driver `SdStorage` (VSPI: SCK=18, MISO=19, MOSI=23, CS=5; FAT32).
- `logEvent(tipo, origen, timestamp)` en formato CSV/JSON línea a línea.
- **Mutex** sobre la SD (regla "Siempre"): ninguna tarea escribe sin tomar el semáforo.
- Reloj: `millis()` en demo; nota para RTC/NTP en producción.

**Hardware requerido (compra):** segunda microSD (independiente de la del DFPlayer), ambas FAT32.
**Hecho cuando:** cada evento queda registrado con hora, tipo y origen; lectura posterior del archivo coherente.

---

## Fase 4 — SIM800L: activación por SMS (diferenciador clave)
**Objetivo:** disparar la alarma por SMS sin internet.
**Requisitos que cubre (fuente):** "activación por SMS a la central si no hay internet (GPRS)"
(`bases §4`); módulo GSM-GPRS cuatribanda con control AT (`bases §3.3`). Secuencia AT, push `CNMI` y
parseo de `+CMT:` → `at-sim800l`. Reglas de alimentación/divisor del SIM800L → `hw-skill`.

**Firmware**
- Interfaz `IGsm` + driver `Sim800Driver` (UART2: RX=26 ← SIM_TX, TX=27 → SIM_RX vía divisor; reset opc. GPIO33).
- Secuencia AT: `AT`, `ATE0`, `AT+CMGF=1`, `AT+CNMI=2,2,0,0,0` (push de SMS al serial).
- Parseo de `+CMT:` → texto (`PANICO`, `SOSPECHOSO`…) → `EventType` → publicar en `eventQueue`.
- **Allow-list de remitentes** en `Config.h` (rechazar números no autorizados).
- `TaskGsm` con **mutex de UART**; diagnóstico con `AT+CSQ`, `AT+CREG?`.

**Hardware requerido (compra):**
- **Rail dedicado para el SIM800L** (causa #1 de fallos) → ver "Pendientes de compra".
- Condensador bulk 1000–2200 µF + cerámico 100 nF en VCC del SIM800L.
- Divisor de nivel ESP32 TX→SIM RX (1 kΩ + 2.2 kΩ ≈ 2.8 V).
- Antena GSM uFL + SIM 2G activa (probada antes en un teléfono 2G).

**Hecho cuando:** un SMS `PANICO` desde un número de la allow-list dispara el audio y queda logueado; un SMS de número no autorizado se ignora; el módulo registra (`AT+CREG? → 0,1`) sin reiniciarse al transmitir.

---

## Fase 5 — WiFi Station + SoftAP y configuración inalámbrica
**Objetivo:** configurar el equipo sin botones ni jumpers.
**Requisitos que cubre (fuente):** "configurable de forma **inalámbrica en cualquier momento** (sin
botones/jumpers)" (`bases §1`); el módulo WiFi opera AP+Station (`bases §3.1`). Persistencia y secrets
fuera de Git → `fw-skill`.

**Firmware**
- `TaskNet`: WiFi en modo **Station + SoftAP** simultáneo.
- Portal de configuración por SoftAP (SSID/clave, allow-list, parámetros) servido por la ESP32.
- Persistencia en **NVS/Preferences** (no recompilar para cambiar credenciales).
- Credenciales reales fuera de Git (`secrets.h` o `Config.h` ignorado).

**Hardware:** ninguno nuevo (WiFi integrado).
**Hecho cuando:** desde un teléfono se entra al SoftAP, se cambian credenciales y persisten tras reinicio.

---

## Fase 6 — Web Service REST / JSON en la ESP32
**Objetivo:** exponer la operatividad como REST/JSON.
**Requisitos que cubre (fuente):** "operatividad completa disponible como **Web Service tipo REST**"
(`bases §1`); operaciones REST+JSON: estado de alarma, activar/desactivar audio, listar alarmas
(`bases §5`). oAuth2.0/HTTPS quedan para backend/4G (nota de `at-sim800l`).

**Firmware**
- Servidor HTTP en la ESP32 (`WebServer`) con endpoints JSON, por ejemplo:
  - `GET /api/estado` → estado actual + RSSI + tipo de red.
  - `POST /api/alarma` → dispara por tipo (publica evento).
  - `POST /api/audio` / `POST /api/silenciar`.
  - `GET /api/eventos` → últimos registros de la SD.
- Cada request entrante se convierte en `EventType::REST_IN` y entra a la **misma cola** (no lógica duplicada).
- Nota de producción: oAuth2.0 + HTTPS quedan para backend/4G (ver Fase 7); en demo es HTTP local.

**Hardware:** ninguno nuevo.
**Hecho cuando:** `curl` a los endpoints opera la alarma y devuelve JSON coherente.

---

## Fase 7 — GPRS / HTTP saliente (SIM800L → webhook)
**Objetivo:** notificar eventos al exterior cuando solo hay 2G.
**Requisitos que cubre (fuente):** "notificación a **webhook**" y operaciones del web service por la ruta
GPRS (`bases §5`); GPRS clase B del módulo GSM (`bases §3.3`). Comandos `SAPBR`/`HTTP` → `at-sim800l`.

**Firmware**
- En `Sim800Driver`: portadora `AT+SAPBR` (APN de la operadora), `AT+HTTPINIT` → POST JSON (`AT+HTTPACTION=1`).
- El controlador, ante un evento, publica al backend por la red disponible.
- Manejo de error y reintento; no bloquear `TaskController` (el POST vive en `TaskGsm`).
- Nota: HTTPS nativo del SIM800L es limitado → en producción TLS por backend intermediario o módulo 4G/Cat-M (SIM7000/SIM7600), AT casi idéntico.

**Hardware:** SIM con plan de datos 2G (la misma de la Fase 4).
**Hecho cuando:** un evento genera un POST JSON recibido por un endpoint de prueba.

---

## Fase 8 — Tercer modo de conectividad: LAN W5500 (RJ45)
**Objetivo:** completar **WiFi + LAN + GPRS** que exige el ANEXO 1.
**Requisitos que cubre (fuente):** "conectividad LAN, WiFi, GPRS — tres modos de Ethernet" (`bases §1`);
módulo LAN RJ45 por SPI con TCP/IP por hardware (`bases §3.2`). W5500 sobre ENC28J60 y CS compartido con
la SD → `hw-skill`.

**Firmware**
- Módulo **W5500** en el bus SPI compartido con la microSD, con **CS distinto** (ej. GPIO4); gestionar cada CS por separado.
- Cliente Ethernet para los mismos endpoints/POST que WiFi y GPRS (reutilizar la capa de red detrás de una interfaz).

**Hardware requerido (compra):** módulo **LAN W5500** (preferir sobre ENC28J60).
**Hecho cuando:** con cable RJ45 (WiFi apagado) el equipo opera REST y envía eventos.

---

## Fase 9 — Multiconectividad, watchdog y robustez
**Objetivo:** seleccionar red automáticamente y mantener el sistema vivo.
**Requisitos que cubre (fuente):** notificación de "tipo de red (WiFi/LAN/GPRS) y **RSSI**" y de
conectado/desconectado (`bases §4`). Tareas FreeRTOS, watchdog y checklist de revisión → `fw-skill`.

**Firmware**
- "Connectivity manager": prioridad WiFi → LAN → GPRS con conmutación automática (failover).
- `TaskWatchdog`: heartbeat de cada tarea; reinicio controlado si una se cuelga.
- Reporte de estado: tipo de red activa, RSSI (`AT+CSQ` / WiFi RSSI), conectado/desconectado.
- Repaso final contra el **checklist de revisión** de la skill de firmware (criterios "Siempre" + "Por defecto").

**Hardware:** ninguno nuevo.
**Hecho cuando:** al caer una red, el equipo conmuta a otra y lo reporta; el watchdog recupera tareas colgadas.

---

## Mapa fase → demostrable de las bases
Los cinco demostrables del "primer funcionamiento" quedan cubiertos así:
1. Config inalámbrica (Station + SoftAP) → **Fase 5**
2. Web Service REST/JSON → **Fase 6**
3. Audio por tipo de evento → **Fase 2**
4. Activación por SMS sin internet → **Fase 4** *(diferenciador)*
5. Log en microSD → **Fase 3**

Para una **demo mínima convincente**, el camino más corto es: Fase 0 → 1 → 2 → 3 → 4 → 5 → 6.
Las fases 7–9 elevan el cumplimiento hacia producción del lado ESP32.

---

# Pendientes de compra (análisis y recomendación)

**Ya disponible:** ESP32 DevKit V1, SIM800L, DFPlayer Mini, módulo MicroSD.

## Imprescindible para la demo

| Componente | Fase | Recomendación | Por qué |
|------------|------|---------------|---------|
| **Rail de alimentación dedicado SIM800L** | 4 | **Buck MP1584 ajustado a ~4.0 V** desde 5–9 V | Es la **causa #1 de fallos**. El buck desde una fuente de pared da una demo reproducible y estable. Alternativa: Li-ion 18650 (3.7–4.2 V) si se quiere autonomía, pero exige carga/gestión. **Nunca** alimentar del 3V3 del ESP32. |
| **Condensador 1000–2200 µF** + 100 nF cerámico | 4 | Electrolítico 1500 µF/6.3 V+ junto a VCC del SIM800L | Absorbe los picos de 2 A del transmisor; sin él hay brownouts y reinicios. |
| **Resistencias divisor** 1 kΩ + 2.2 kΩ | 4 | Par para ESP32 TX → SIM RX (~2.8 V) | El RX del SIM800L no siempre tolera 3.3 V. |
| **Resistencia 1 kΩ** | 2 | En serie ESP32 TX → DFPlayer RX | Reduce ruido/clics en el audio. |
| **Parlante 4–8 Ω, ≤3 W** | 2 | Pequeño, con el amp interno del DFPlayer | Sin él no se prueba nada de audio. |
| **2× microSD** | 2, 3 | Dos tarjetas FAT32 (una audios, una log) | Son **zócalos distintos**: DFPlayer lee audios, ESP32 registra eventos. No compartir. |
| **Antena GSM uFL** | 4 | Antena roscable/uFL para SIM800L | Sin antena el registro 2G falla intermitentemente. |
| **SIM 2G activa con datos** | 4, 7 | Operadora con 2G vigente en Ecuador; **probar en teléfono 2G antes** | Si la operadora apaga 2G, GPRS/SMS no funcionan. |
| **Pulsador + protoboard/cables/Dupont** | 1 | Pulsador NA + protoboard | Para el botón de pánico y el cableado de la demo. |

## Para cumplimiento completo del lado ESP32

| Componente | Fase | Recomendación | Por qué |
|------------|------|---------------|---------|
| **Módulo LAN W5500** | 8 | **W5500** sobre ENC28J60 | Tercer modo de conectividad (WiFi + **LAN** + GPRS) que exige el ANEXO 1; mejor stack y rendimiento que el ENC28J60. Comparte SPI con la microSD usando otro CS. |

## Recomendación de alimentación (decisión)
Para la demo: **fuente de pared 5 V/≥3 A → buck MP1584 a 4.0 V → SIM800L** (con el condensador bulk en VCC),
y el ESP32 por su USB. Es la opción más estable y repetible para mostrar el "primer funcionamiento".
La batería 12 V 7 A, la fuente 120–240 VAC y la etapa de potencia de 200 W son de **producción** y quedan
fuera del alcance de la ESP32.

## Mirando a producción (no comprar para la demo, sí documentar)
- **Módulo GSM homologado ARCOTEL** (o salto a 4G/Cat-M SIM7000/SIM7600): el SIM800L "pelón" sirve para demo, no para producción.
- **RTC** (DS3231) o NTP para marcas de tiempo fiables en el log.
- Reemplazo del GPIO de disparo por la **etapa de potencia real** (2 relés de estado sólido).
