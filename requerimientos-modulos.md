# Requerimientos de módulos — Prototipo ESP32

Lo que falta comprar para el prototipo del Sistema de Alarmas Comunitarias, acotado a la **ESP32 y sus
periféricos directos**. Ver fases en [instrucciones.md](instrucciones.md).

**Ya disponible:** ESP32 DevKit V1, SIM800L, DFPlayer Mini, módulo MicroSD.

**Fuentes:** `bases §N` → `references/bases-anexo1.md` (ANEXO 1) · `hw-skill` → `alarmas-comunitarias-hw/SKILL.md` ·
`at-sim800l` → referencia del SIM800L.

## Tabla de compra

La columna **Etapa** separa lo del primer prototipo (Demo) de lo que solo aplica en Producción
(no comprar ahora; no depende del microcontrolador).

| # | Módulo | Spec mínima / modelo | Cant. | Fase | Etapa | Justificación (fuente) |
|---|--------|----------------------|-------|------|-------|------------------------|
| A1 | Buck rail SIM800L | MP1584EN; in 5–9 V, out ~4.0 V, ≥2 A | 1 | 4 | Demo | Alimentación = causa #1 de fallos; nunca del 3V3 (`hw-skill`, `bases §3.3`) |
| A2 | Condensador bulk | 1500 µF ≥6.3 V + 100 nF cerámico | 1+1 | 4 | Demo | Absorbe picos de 2 A; sin él reinicia (`at-sim800l`) |
| A3 | Divisor de nivel | 1 kΩ + 2.2 kΩ (→ ~2.8 V) | 1 par | 4 | Demo | RX del SIM800L no tolera 3.3 V (`hw-skill`) |
| A4 | Resistencia DFPlayer | 1 kΩ serie en RX | 1 | 2 | Demo | Reduce ruido/clics en audio (`hw-skill`) |
| A5 | Parlante | 4–8 Ω, ≤3 W | 1 | 2 | Demo | Sin él no se prueba audio (`hw-skill`) |
| A6 | MicroSD ×2 | 2–16 GB clase 10, FAT32 | 2 | 2,3 | Demo | Zócalos distintos: audios y log (`hw-skill`) |
| A7 | Antena GSM | uFL, cuatribanda 850–1900 | 1 | 4 | Demo | Sin antena el registro 2G falla (`at-sim800l`) |
| A8 | SIM 2G | GSM+GPRS, micro-SIM, APN operadora | 1 | 4,7 | Demo | SMS y GPRS/HTTP (`bases §4,§5`); probar antes |
| A9 | Pulsador + protoboard | NA + jumpers Dupont, GND común | 1+kit | 1 | Demo | Botón de pánico de la demo (`hw-skill`) |
| B1 | Módulo LAN W5500 | W5500 SPI, RJ45 metálico, 3.3 V | 1 | 8 | Demo | 3.er modo WiFi+LAN+GPRS (`bases §1,§3.2`) |
| P1 | Módulo GSM homologado | Homologado ARCOTEL o 4G/Cat-M (SIM7000/7600) | 1 | — | Producción | SIM800L no homologado (`bases §3.3`) |
| P2 | Fuente + batería | 120–240 VAC→12 V ≥100 W + batería 12 V 7 A | 1 | — | Producción | Alimentación y respaldo (`bases §2`) |
| P3 | Etapa de potencia | 2 relés est. sólido ~200 W, sirena 30 W, estrobo, 2 bocinas 90 W | 1 | — | Producción | Disuasión real (`bases §2,§3.4`) |
| P4 | Gabinete + PCB | Acero IP65; PCB bajo normas IPC | 1 | — | Producción | Intemperie y manufactura (`bases §2,§3`) |
| P5 | RTC | DS3231 (o NTP) | 1 | — | Producción | Marca de tiempo fiable en el log (recomendado) |
