# SIM800L — Comandos AT esenciales

Comunicación serial a **9600 bps** (autobaud variable; fijar con `AT+IPR=9600`). Toda línea termina en `\r`. El módulo responde `OK` / `ERROR`.

## Arranque y diagnóstico
| Comando | Para qué |
|---------|----------|
| `AT` | Handshake (debe responder `OK`) |
| `ATE0` | Apagar eco (recomendado en firmware) |
| `AT+CPIN?` | Estado de la SIM (`+CPIN: READY`) |
| `AT+CSQ` | Calidad de señal (0–31; >10 aceptable, 99 = sin señal) |
| `AT+CREG?` | Registro en red (`0,1` local / `0,5` roaming) |
| `AT+CGATT?` | Adjunto a GPRS (`1` = sí) |
| `AT+COPS?` | Operadora registrada |

> Si `AT` no responde o el módulo se reinicia al transmitir → **es alimentación** (rail <3.4 V o sin condensador), no firmware.

## SMS — recepción para ACTIVACIÓN (clave del demo)
| Comando | Para qué |
|---------|----------|
| `AT+CMGF=1` | Modo texto |
| `AT+CNMI=2,2,0,0,0` | **Push** del SMS entrante directo al serial (`+CMT:`) |
| `AT+CMGR=n` | Leer el SMS en índice n |
| `AT+CMGD=n` | Borrar SMS (limpiar memoria) |

Flujo sugerido: con `CNMI=2,2,...` el ESP32 recibe `+CMT: "+593...","","fecha"` y en la línea siguiente el texto (ej. `PANICO`). El firmware parsea el texto → dispara el audio en el DFPlayer. Limitar remitentes a una allow-list.

## SMS — envío (notificación)
1. `AT+CMGF=1`
2. `AT+CMGS="+593XXXXXXXXX"` → esperar el prompt `>`
3. Escribir el texto → enviar `Ctrl+Z` (0x1A).

## GPRS / HTTP (ruta hacia REST de las bases)
| Comando | Para qué |
|---------|----------|
| `AT+SAPBR=3,1,"CONTYPE","GPRS"` | Tipo de portadora |
| `AT+SAPBR=3,1,"APN","<apn_operadora>"` | APN (consultar a la operadora) |
| `AT+SAPBR=1,1` | Abrir portadora |
| `AT+SAPBR=2,1` | Ver IP asignada |
| `AT+HTTPINIT` | Iniciar HTTP |
| `AT+HTTPPARA="URL","http://..."` | URL destino |
| `AT+HTTPPARA="CONTENT","application/json"` | Cabecera JSON |
| `AT+HTTPDATA=<len>,<ms>` | Cargar cuerpo JSON |
| `AT+HTTPACTION=1` | POST (0=GET, 1=POST) |
| `AT+HTTPREAD` | Leer respuesta |
| `AT+HTTPTERM` / `AT+SAPBR=0,1` | Cerrar |

> HTTPS nativo en el SIM800L es limitado. Para el oAuth2.0/HTTPS de las bases, en producción conviene un módulo 4G/Cat-M (SIM7000/SIM7600) o que el TLS lo maneje un backend intermediario. En el demo, GPRS suele ir por HTTP a un endpoint propio.

## Notas
- Dar ~3–5 s tras energizar antes del primer `AT` (registro en red).
- Una SIM 2G activa y con saldo/plan de datos; probarla antes en un teléfono.
- Antena GSM en el uFL: sin antena el registro falla intermitentemente.
