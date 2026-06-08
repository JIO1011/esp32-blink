---
name: alarmas-comunitarias-hw
description: "Consulta esta skill SIEMPRE que se trabaje en el firmware, hardware o arquitectura del Sistema de Alarmas Comunitarias / Botón de Pánico (proyecto COMSECA-Ambato) con ESP32, SIM800L, DFPlayer Mini (TF-16P) o módulo MicroSD. Úsala antes de proponer circuitos, pinouts, esquemas de alimentación, código de firmware o decisiones de conectividad, y siempre que haya que verificar que una propuesta cumpla las bases del ANEXO 1 (REST/JSON, multiconectividad WiFi/LAN/GPRS, activación por SMS, audios por tipo de evento, configuración inalámbrica, respaldos). Actívala incluso si el usuario no menciona 'las bases' explícitamente: toda propuesta de hardware o firmware de este proyecto debe pasar por aquí."
---

# 📟 Alarmas Comunitarias — Hardware & Firmware v1.0

## Propósito
Fuente única de verdad para diseñar el prototipo del **Sistema de Alarmas Comunitarias y Botón de Pánico** (ANEXO 1 — COMSECA / Ambato) sin perder de vista **las bases** del pliego, y proponiendo siempre la solución técnicamente correcta.

**Dos reglas que nunca se rompen:**
1. **Ninguna propuesta de hardware o firmware se entrega sin contrastarla con las bases** (checklist más abajo y `references/bases-anexo1.md`).
2. **Lo que cumple en el demo ≠ lo que cumple en producción.** Cada vez que el prototipo "simplifica" un requisito, hay que decirlo explícitamente y anotar qué falta para la versión definitiva.

---

## Cuándo usar esta skill
- Diseñar/revisar el pinout, la alimentación o el esquema eléctrico.
- Escribir o revisar firmware (ESP32 + SIM800L + DFPlayer + MicroSD).
- Decidir conectividad (WiFi / LAN / GPRS / SMS) o protocolo (REST, MQTT).
- Preparar la demo de "primer funcionamiento".
- Validar que algo **cumple el ANEXO 1** antes de comprometerlo.

---

## Hardware del prototipo (lo que hay)

| Módulo | Rol en el sistema | Punto crítico |
|--------|-------------------|---------------|
| **ESP32 DevKit V1 (WROOM-32)** | Cerebro: WiFi+BT, lógica de eventos, servidor REST, cliente HTTP/MQTT | Es el "Módulo Wifi/Bluetooth" de las bases. 3 UART HW, 3.3 V lógico |
| **SIM800L (GSM/GPRS)** | Conectividad 2G + **activación por SMS** sin internet | Alimentación 3.4–4.4 V con picos de **2 A**. Mata el proyecto si se hace mal |
| **DFPlayer Mini (TF-16P)** | Reproduce los audios pregrabados por tipo de evento | SD **propia** para los `.mp3`. Amp interno 3 W para el parlante de demo |
| **Módulo MicroSD (SPI)** | Almacenamiento del ESP32: **log/auditoría** de eventos y config | SD **independiente** de la del DFPlayer. FAT32/FAT16 |

> ⚠️ **Dos tarjetas SD distintas.** El DFPlayer lee los audios desde **su propio** zócalo. El módulo MicroSD del bus SPI es para que **el ESP32** registre eventos y respaldos. No intentes compartir una sola tarjeta entre ambos.

---

## Materiales — lo que falta comprar
Lo que ya hay: ESP32 DevKit V1, SIM800L, DFPlayer Mini, módulo MicroSD.

**Imprescindible para el demo (sin esto no funciona):**

| Material | Para qué | Detalle |
|----------|----------|---------|
| Parlante 4–8 Ω, ≤3 W | Escuchar los audios del DFPlayer | Sin él no se prueba nada de audio |
| Fuente/buck dedicado para SIM800L | Rail propio de 4 V a ≥2 A | MP1584 o LM2596 ajustado a ~4.0 V, **o** Li-ion 18650 + portapilas. **Causa #1 de fallos si falta** |
| Condensador 1000–2200 µF | Absorber los picos del SIM800L | Va en VCC del SIM800L, junto a la fuente anterior |
| Resistencia 1 kΩ | Línea TX→RX del DFPlayer | Evita ruido/clics |
| Resistencias 1 kΩ + 2.2 kΩ | Divisor TX(ESP32)→RX(SIM800L) | Baja 3.3 V a ~2.8 V |
| 2× microSD | Una para audios (DFPlayer), otra para log (ESP32) | Ambas FAT32 |
| Antena GSM (uFL) | Registro del SIM800L en red | Sin antena el módulo se cae |
| SIM 2G activa | Conectividad GSM/GPRS | **Probarla en un teléfono 2G antes de comprar el plan** |

**Para cumplir las bases completas (tercer modo de conectividad — se añade después del demo básico):**

| Material | Para qué | Detalle |
|----------|----------|---------|
| **Módulo LAN W5500** | Conexión por cable RJ45 (las bases exigen WiFi **+ LAN +** GPRS) | SPI, comparte bus con la microSD usando un CS distinto (ej. GPIO4). Preferir W5500 sobre ENC28J60 |

---

## Las bases en una página (checklist de cumplimiento)
Verifica cada propuesta contra esto. Detalle completo en `references/bases-anexo1.md`.

### Central de alarma (ítem 1)
- [ ] Microcontrolador 32 bits dual-core, WiFi AP+Station, BT, TCP/IP → **ESP32 ✓**
- [ ] Conectividad **WiFi + LAN(RJ45) + GPRS** (tres modos) → demo cubre WiFi+GPRS; **LAN pendiente (W5500)**
- [ ] **Configurable de forma inalámbrica en cualquier momento** (sin botones/jumpers físicos) → SoftAP de config
- [ ] Audio configurado **por tipo de evento**, mínimo: pánico, sospechoso, disuasivo, pregrabado
- [ ] **Mínimo 2 audios automáticos** desde app/web
- [ ] Etapa de potencia: 2 relés estado sólido, salida ~200 W, control para sirena/estrobo → **diferido a producción** (demo usa amp 3 W)
- [ ] Respaldo de BD automático + envío quincenal al administrador
- [ ] **Web Service tipo REST/JSON** equivalente a toda la operatividad (oAuth2.0, https)

### Botón de auxilio / App (ítem 1 y 2)
- [ ] App nativa Android/iOS/Windows; PWA; repos Play Store / App Store
- [ ] **Activación por SMS hacia la central (GPRS)** cuando no hay internet → **SIM800L ✓**
- [ ] Geolocalización en tiempo real, error ≤ 9 m, "Función sígueme" ≥ 1 h
- [ ] Notificación en segundo plano con ubicación en mapa
- [ ] Activar la **central más cercana por geocercas**
- [ ] Chat comunitario con BOT, imágenes/videos ≤ 16 MB
- [ ] Aceptación de uso de datos descargable (PDF) + Ley de Protección de Datos
- [ ] Botón físico IP65, RF 433 MHz, batería de respaldo, cifrado (AES/DES)

### Software de gestión / monitoreo
- [ ] Roles (admin/monitoreo/ingreso), 2FA para administradores
- [ ] Notificaciones: alarma activada/silenciada, tapa abierta, batería baja, sin energía >5 min, conectividad y RSSI, audio en curso
- [ ] Reportes export Excel/PDF; búsqueda inteligente por cualquier dato
- [ ] Agrupar barrios en comunidad (circuitos integrados para eventos masivos)
- [ ] Servidores redundantes alta disponibilidad, **al menos uno fuera del país**; respaldos diarios/semanales/mensuales
- [ ] Volcado de datos a **AWS** (observatorio de seguridad de Ambato)

### Banderas de cumplimiento que NO se pueden olvidar
- 🚩 **Homologación ARCOTEL del módulo GSM.** El SIM800L "pelón" **no está homologado**: sirve para el demo, **no para producción**. Documentar el módulo homologado (o modem) que irá en la versión final.
- 🚩 **2G en Ecuador.** Verificar cobertura/continuidad 2G de la operadora antes de confiar en GPRS/SMS. Si hay riesgo de apagado 2G, la ruta de producción es un módulo Cat-M/4G (ej. SIM7000/SIM7600), control AT casi idéntico.
- 🚩 **PCB bajo normas IPC**, IP65, antenas WIFI/GSM/RF roscables, brandeo → fase de gabinete, no del demo.

---

## Reglas de oro del hardware (no negociables)

### 1. Alimentación del SIM800L = causa #1 de fallos
- Rail **dedicado 3.4–4.4 V** (objetivo ~4.0 V), capaz de **≥ 2 A** de pico.
- **NUNCA** alimentarlo desde el pin 3V3 del ESP32: provoca brownouts y reinicios.
- **Condensador bulk de 1000–2200 µF** justo en VCC del SIM800L (más un cerámico 100 nF).
- Opciones válidas: Li-ion 18650 (3.7–4.2 V) directo, o buck (MP1584 / LM2596) desde 5–9 V ajustado a ~4.0 V y dimensionado a ≥ 2 A.
- **GND común** entre todos los módulos. El SIM800L exige tierra sólida.

### 2. Niveles lógicos (el ESP32 ayuda: es 3.3 V)
- SIM800L TX (~2.8 V) → ESP32 RX (3.3 V): **OK directo**.
- ESP32 TX (3.3 V) → SIM800L RX: usar **divisor** (~1 kΩ serie + 2.2 kΩ a GND ≈ 2.8 V) o level shifter. El RX del SIM800L no siempre tolera 3.3 V.
- DFPlayer RX: **1 kΩ en serie** desde el TX del ESP32 (reduce ruido/clics; obligatorio si alguna vez se usa a 5 V de lógica).

### 3. UART por hardware, nunca SoftwareSerial
El ESP32 tiene 3 UART. GPRS es sensible al timing; SoftwareSerial es inestable.
- **UART0** (GPIO1/3): reservado para USB/debug. No usarlo para módulos.
- **SIM800L** → UART HW remapeada (ej. RX=GPIO26 ← SIM_TX, TX=GPIO27 → SIM_RX).
- **DFPlayer** → UART HW (ej. RX=GPIO16 ← DF_TX, TX=GPIO17 → DF_RX, a 9600 bps).
- Evitar: GPIO6–11 (flash), pines strapping (0,2,12,15) y solo-entrada (34–39) para TX.

### 4. MicroSD por SPI
- VSPI por defecto: **SCK=18, MISO=19, MOSI=23, CS=5**. Librería `SD.h`/`SPI.h`. FAT32.
- Si luego se añade LAN (W5500), comparte el bus SPI con **otro CS** (ej. GPIO4) y se gestiona cada CS por separado.

---

## Pinout sugerido del prototipo

| Señal | ESP32 | Notas |
|-------|-------|-------|
| SIM800L TX → ESP32 RX | GPIO26 | directo |
| ESP32 TX → SIM800L RX | GPIO27 | **vía divisor a ~2.8 V** |
| SIM800L RST (opc.) | GPIO33 | reset por software |
| DFPlayer TX → ESP32 RX | GPIO16 | |
| ESP32 TX → DFPlayer RX | GPIO17 | **1 kΩ en serie** |
| SD SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 | VSPI, FAT32 |
| Relé/estrobo (placeholder demo) | GPIO13 | simula la etapa de potencia |
| LED estado | GPIO2 | latido / conexión |

*Asume ESP32-WROOM (DevKit V1). En módulos WROVER los GPIO16/17 los ocupa la PSRAM: en ese caso reasignar DFPlayer a 32/33 y SIM800L a 25/26.*

---

## Convenciones del DFPlayer Mini (audios)
- Tarjeta SD del DFPlayer en **FAT32**.
- Carpeta `/mp3` en la raíz; archivos con **4 dígitos** al inicio: `0001.mp3`, `0002.mp3`…
- Mapa de eventos sugerido (alinear con los "tipos de alarma" de las bases):
  - `0001` = Pánico/Emergencia
  - `0002` = Sospechoso
  - `0003` = Disuasivo
  - `0004` = Libadores
  - `0005` = Mantenimiento
- Carpeta `/advert` para anuncios que pausan y reanudan (útil para perifoneo intercalado).
- Control por UART 9600 bps (DFRobotDFPlayerMini o comandos crudos). Esperar ~1–2 s tras `begin()` antes del primer `play`.

---

## DEMO vs. Producción (detalle por subsistema)

**Regla de fondo:** el demo demuestra que la lógica y la electrónica funcionan; la producción cumple el ANEXO 1 al 100 %. Cada fila dice qué hace el demo y qué falta para producción. **Cuando propongas algo, di siempre en qué columna está.**

| Subsistema | En el DEMO (suficiente para mostrar) | En PRODUCCIÓN (lo que exige el ANEXO 1) |
|------------|--------------------------------------|------------------------------------------|
| **Cerebro** | ESP32 DevKit V1 sobre protoboard | Mismo SoC, pero en PCB propia bajo normas IPC |
| **Conectividad** | WiFi (Station + SoftAP) + GPRS/SMS | **WiFi + LAN(W5500) + GPRS**, los tres modos |
| **Configuración** | SoftAP web para credenciales | Igual, configurable inalámbrica en cualquier momento |
| **API** | Endpoint REST/JSON local en el ESP32 | REST/JSON completo, **oAuth2.0 + HTTPS**, webhooks |
| **Audio** | DFPlayer + amp interno 3 W → parlante chico | Audio del DFPlayer/MCU → **amplificador 200 W + 2 bocinas 90 W** |
| **Potencia/disuasión** | 1 GPIO simula la activación (LED/relé chico) | **2 relés de estado sólido**, sirena 30 W, luz estroboscópica |
| **Módulo GSM** | SIM800L "pelón" sobre 2G | Módulo GSM **homologado ARCOTEL** (o salto a 4G/Cat-M: SIM7000/SIM7600) |
| **Energía** | USB + rail dedicado para el SIM800L | Fuente 120–240 VAC → 12 V ≥100 W + **batería respaldo 12 V 7 A** |
| **Almacenamiento** | Log local en microSD del ESP32 | Respaldo BD automático, envío quincenal al administrador |
| **Gabinete** | Sin gabinete (protoboard) | Metálico **IP65**, doble fondo, cerradura triangular, antenas roscables, brandeo |
| **Backend / nube** | No hay (todo local en el ESP32) | Servidores redundantes (1 fuera del país), 2FA, reportes, **volcado a AWS** |
| **Botón de pánico** | App/REST dispara la alarma | Botón físico **IP65 + RF 433 MHz**, batería respaldo, señal cifrada (AES/DES) |

### Qué demostrar concretamente en el "primer funcionamiento"
Estos cinco puntos cubren los requisitos más distintivos de las bases:
1. ESP32 en **Station + SoftAP** → "config inalámbrica sin botones ni jumpers".
2. **Endpoint REST/JSON** + mini-web servida por el ESP32 → "Web Service tipo REST".
3. Evento → DFPlayer reproduce el **audio según el tipo** → "audio por evento" y "≥2 audios".
4. **Activación por SMS** vía SIM800L → "sin internet, activación por SMS/GPRS". *Diferenciador fuerte.*
5. **Log en microSD** (hora, tipo, origen) → auditoría/respaldo local.

---

## Flujo de trabajo: "siempre la mejor solución"
Cuando te pidan diseñar o revisar algo de este proyecto, sigue este orden:

1. **Ubica el requisito en las bases.** ¿Qué exige el ANEXO 1? (checklist + `references/bases-anexo1.md`).
2. **Aplica las reglas de oro del hardware** (alimentación, niveles, UART HW, SPI).
3. **Marca demo vs. producción.** Si simplificas algo, dilo y anota qué falta.
4. **Verifica equipo de bring-up** disponible (osciloscopio, multímetro, fuente de banco, estación de soldadura están en el inventario del lab → skill `lab-inventory`).
5. **Propón, justifica y lista riesgos.** Nada de afirmaciones sin contraste con las bases.

### Criterios para elegir entre alternativas
- **Cumple las bases > es cómodo.** Si una opción cómoda incumple un requisito, decláralo.
- **Fiabilidad en demo > completitud.** En el demo, prioriza lo que funciona de forma estable y reproducible frente a meter todas las funciones.
- **Camino de migración.** Prefiere decisiones cuyo salto a producción sea suave (ej. AT del SIM800L ≈ AT del SIM7600).

---

## Riesgos típicos a vigilar (de mayor a menor)
1. **SIM800L se reinicia / no registra** → casi siempre alimentación (rail débil o sin condensador).
2. **2G sin cobertura** de la operadora → probar la SIM en un teléfono 2G antes del demo.
3. **DFPlayer no lee** → SD mal formateada, nombres sin 4 dígitos, o falta carpeta `/mp3`.
4. **Choque de pines** → strapping/flash/solo-entrada mal usados.
5. **Ruido/clics en audio** → falta el 1 kΩ en RX del DFPlayer o tierra ruidosa.

---

## Referencias (van empaquetadas dentro de esta skill)
Estos dos archivos viven **dentro del `.skill`**. Claude Code los abre automáticamente cuando los necesita; tú no tienes que manejarlos ni abrirlos a mano.
- `references/bases-anexo1.md` — Requisitos completos del ANEXO 1 (central, botón, software, webservices). Consultar antes de cerrar cualquier decisión de cumplimiento.
- `references/at-sim800l.md` — Comandos AT esenciales para registro, SMS (recepción para activación) y GPRS. Consultar al escribir o depurar el firmware del SIM800L.
