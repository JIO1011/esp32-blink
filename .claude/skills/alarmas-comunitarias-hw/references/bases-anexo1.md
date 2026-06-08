# Bases — ANEXO 1 (Alarmas y Botones de Pánico)
Extracto estructurado del pliego. Es la referencia de cumplimiento. **Ante duda, prevalece el pliego original.**

## Índice
1. Características básicas de la central
2. Componentes y características mínimas
3. Tarjeta electrónica y módulos
4. Software de gestión y app
5. Webservices / interoperabilidad
6. Servidores y respaldos
7. Botón de emergencia (ítem 2)

---

## 1. Características básicas de la central (ítem 1, cantidad 500)
- Alimentación 120–240 VAC; respaldo batería **12 V 7 A**.
- 1 sirena 30 W; 2 bocinas 90 W; luz estroboscópica naranja.
- Conectividad: **LAN, WiFi, Bluetooth, GPRS** (tres modos de Ethernet: WiFi, RJ45, GPRS por GSM).
- Envío de mensajes audibles automáticos desde app móvil/PC y desde web de monitoreo (pregrabados o en línea).
- Audio configurado **por tipo de evento**, mínimo: pánico, sospechoso, pregrabados, disuasivo.
- 2 bocinas de perifoneo.
- Operatividad completa disponible como **Web Service tipo REST**.
- Configurable de forma **inalámbrica en cualquier momento** (sin botones/jumpers).

## 2. Componentes y características mínimas
- **Gabinete:** acero galvanizado 0.7–1.2 mm, corte láser, IP65, doble fondo, cerradura triangular, prensaestopas, soportes (6), conectores de antena WIFI/GSM/RF roscables, pintura electrostática, brandeo institucional.
- **Fuente:** AC100–240 V 50/60 Hz, salida 12 V DC, ≥100 W, ≥5 A, LED, protección cortocircuito, CE/EAC, −25 a 70 °C, bornera.
- **Batería:** 12 V 7 A (respaldo en cortes).
- **Bocinas:** tipo acoustic redonda con driver, 90 W reales, aluminio.
- **Luz estroboscópica:** 110 V. **Sirena doble tono:** 12 V, 30 W, blanco.

## 3. Tarjeta electrónica y módulos (PCB bajo normas IPC)
Módulo principal microcontrolado con tres modos: WiFi, LAN RJ45, GPRS. ICSP para firmware. 2 relés estado sólido, 4 E/S digitales, 2 ADC, 2 PWM, fuentes 5 V, salidas de audio estéreo, control de amplificador de perifoneo, cargador de batería, indicadores.

### 3.1 Módulo WiFi/Bluetooth  → **equivale al ESP32**
Dual-core 32 bits, 80–240 MHz, 500 KB RAM, flash ext. hasta 16 MB, TCP/IP integrado, AP+Station, 802.11 b/g/n (WPA/WPA2), BT 4.2, 36 GPIO, PWM 10 bits, ADC 10 bits, UART/SPI/I2C/I2S, 3.0–3.6 V, ~80 mA, deep sleep.

### 3.2 Módulo LAN RJ45  → **W5500/W5100 (SPI), pendiente en el demo**
Interfaz SPI maestro/esclavo, TCP/IP por hardware, RJ45 8 pines metálico, soporta TCP/UDP/ICMP/IPv4/ARP/IGMP/PPPoE, 3.3 V con tolerancia 5 V en E/S, jumper de alimentación.

### 3.3 Módulo GSM-GPRS  → **equivale al SIM800L** (homologar para producción)
Cuatribanda 850/900/1800/1900, GPRS clase B, control AT (3GPP TS 27.007/27.005 + AT SIMCom), bajo consumo, multi-slot class 12 ~85.6 kbps, LED, pin enable, uFL antena GSM, zócalo micro-SIM desmontable, **jack de audio 4 polos**, 3.6–4.2 V con fuente embebida. **Debe estar homologado ante ARCOTEL.**

### 3.4 Módulo de potencia  → **diferido en el demo**
2 relés estado sólido, entrada de audio, ~200 W reales, control PWM, MOSFET GND común, LED por canal. Controla sirena doble tono y estrobo.

## 4. Software de gestión y app
- App nativa Android/iOS/Windows + PWA; en Play Store y App Store; PC por descarga web o PWA.
- 3 pulsantes principales (Emergencia, Alarma cercana, Silenciosa) + secundarios (sospechoso, intruso, extorsionadores, disuasivo, libadores, pregrabados).
- Un usuario en varias centrales (casa, trabajo…).
- Banner/logo institucional actualizable; responsive; modo claro/oscuro.
- Geolocalización tiempo real **error ≤ 9 m**; "Función sígueme" ≥ 1 h compartida a monitoreo y vecinos.
- **Activación por SMS** a la central si no hay internet (GPRS).
- Activar **central más cercana por geocercas**.
- Chat comunitario con BOT parametrizable, imágenes/videos ≤ 16 MB.
- Aceptación de uso de datos (guardable/descargable PDF); Ley de Protección de Datos vigente.
- Recuperación de contraseña por correo.
- Backup automático de BD; copia íntegra al administrador cada 15 días.
- Gestión: roles, búsqueda inteligente por cualquier dato, CRUD usuarios/centrales/repetidoras/barrios, agrupar barrios en comunidad (circuitos para eventos masivos), bloquear central (mantenimiento), notas de seguimiento por alarma, ver centrales en mapa.
- Notificaciones a operadores: alarma activada (y tipo), silenciada, tapa abierta, batería baja, sin energía >5 min, conectado/desconectado + última conexión, audio en curso, tipo de red (WiFi/LAN/GPRS) y RSSI.
- Reportes export Excel/PDF: alarmas, usuarios, barrios (por fecha/tipo/barrio).
- Aviso de confidencialidad aceptable y descargable.

## 5. Webservices / interoperabilidad
- **HTTPS + oAuth2.0**; notificación a **webhook**.
- Estado de alarma; activar/desactivar audio, sirena, estrobo; ubicación del dispositivo; remitir audios; listar alarmas activas/inactivas.
- Estado/activación de botón; listar botones.
- **REST + JSON** para web, móvil (iOS/Android) y escritorio; autenticación/autorización.
- Operaciones disponibles también en las alarmas que ya posee COMSECA (integración).
- **Volcado de datos a AWS** (observatorio de seguridad de Ambato).

## 6. Servidores y respaldos
- Servidor principal + respaldo en **múltiples continentes** (al menos uno fuera del país), alta disponibilidad, redundancia.
- Sincronización segura y de alta velocidad; plan de recuperación ante desastres con pruebas periódicas.
- Cifrado en reposo y en tránsito, control de acceso por roles, auditorías; **2FA para administradores**.
- Respaldos diarios/semanales/mensuales (BD + código fuente); copia semanal al administrador cada quincena.
- Cumplir normativas (GDPR/HIPAA si aplica).

## 7. Botón de emergencia (ítem 2)
- Energía: red con **batería de respaldo** o baterías con monitoreo de batería baja.
- Indicadores LED + alarmas sonoras de confirmación.
- **IP65**, compacto, resistente a humedad/polvo/golpes.
- Activación: botón físico o app móvil; múltiples destinatarios (emergencias, seguridad, supervisores).
- Configuración fácil (UI o software); pruebas periódicas en línea/campo.
- Conexión: **RF 433 MHz**, WiFi, Bluetooth, GPRS (backup) o Ethernet; protocolo TCP/IP.
- **Señal cifrada (AES/DES)**.
- Recepción RF 433 MHz; zonas inalámbricas (PIR, magnéticos); entrada cableada (2 pulsadores); config por Access Point y remota (web); tipo de alerta (SOS / silenciosa); 12 VDC; LED de estado/conexión; reporte en plataforma.
- Funciones: monitoreable desde la misma central; activable por app o pulsador; alarma **SOS** reflejada en monitoreo; comparte ubicación de instalación al activarse.
