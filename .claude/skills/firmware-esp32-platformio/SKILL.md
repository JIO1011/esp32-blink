---
name: firmware-esp32-platformio
description: "Consulta esta skill SIEMPRE que escribas, estructures, revises o refactorices el firmware del proyecto de Alarmas Comunitarias en ESP32 usando PlatformIO + VSCode. Úsala al crear o modificar drivers (SIM800L, DFPlayer, MicroSD), servicios, controladores o el platformio.ini; al decidir cómo organizar el código, manejar interrupciones, tareas FreeRTOS, máquinas de estado, logging, errores o tests. Es la skill compañera de 'alarmas-comunitarias-hw'. Define dos niveles: criterios 'Siempre' (ISR, mutex, sin delay bloqueante, Config.h — sin excepción) y criterios 'Por defecto' (SRP, DI, interfaces, logger, tests — aplicar salvo razón explícita). Otros métodos son válidos si se justifican; esta skill orienta, no bloquea."
---

# 🧩 Firmware ESP32 con PlatformIO — Buenas Prácticas

## Propósito
Que el firmware del proyecto sea **mantenible durante años**, no un `loop()` gigante. Toda generación o revisión de código pasa por estas reglas. El objetivo no es solo "que encienda", sino practicar arquitectura de firmware profesional.

Skill compañera: **`alarmas-comunitarias-hw`** (hardware, pinout, cumplimiento del ANEXO 1). Si una duda es de *qué construir o si cumple las bases*, va allá. Si es de *cómo escribir el código*, va aquí.

---

## Cuándo usar esta skill
- Crear o tocar el `platformio.ini`, la estructura de carpetas o un componente de `lib/`.
- Escribir o revisar drivers, servicios, controladores, ISR, tareas FreeRTOS.
- Decidir organización, manejo de errores, logging, máquina de estados o tests.
- Cualquier petición de "programar", "ordenar", "limpiar" o "hacer mantenible" el proyecto.

---

## Stack y entorno
- **IDE:** VSCode + extensión PlatformIO.
- **Plataforma/board:** `espressif32` / `esp32doit-devkit-v1` (DevKit V1).
- **Framework:** `arduino` (incluye FreeRTOS). ESP-IDF queda como migración futura.
- **C++17** (`build_flags = -std=gnu++17`), `monitor_speed = 115200`.
- Detalle completo de `platformio.ini`, entornos y testing → `references/platformio-setup.md`.

---

## Estructura del proyecto (PlatformIO)
Las capas se implementan como **componentes en `lib/`** para que cada una compile y se **testee aislada**.

```text
alarmas-fw/
├── platformio.ini
├── include/
│   └── Config.h            # pines, baudrates, constantes (única fuente)
├── src/
│   └── main.cpp            # solo arranque: crea objetos, inyecta, lanza tareas
├── lib/
│   ├── Hal/                # envoltura fina de Arduino (Gpio, Uart) -> testeable
│   ├── Sim800Driver/       # AT/SMS/GPRS detrás de interfaz IGsm
│   ├── Mp3Player/          # DFPlayer detrás de interfaz IAudioPlayer
│   ├── SdStorage/          # MicroSD detrás de interfaz IStorage
│   ├── EventLogger/        # logger con niveles
│   ├── NotificationService/# decide a quién y cómo notificar
│   └── AlarmController/    # máquina de estados, orquesta todo
└── test/
    ├── test_phone/         # corre en env:native, sin ESP32
    └── test_state/
```

**Capas y su única responsabilidad:**

| Capa (`lib/`) | Responsabilidad | Depende de |
|---------------|-----------------|------------|
| `Hal` | Hablar con los periféricos del SoC (GPIO, UART, SPI) | nada (o Arduino) |
| `*Driver` / `Mp3Player` / `SdStorage` | Manejar **un** módulo físico | `Hal` + interfaz propia |
| `EventLogger`, `NotificationService` | Lógica reutilizable, sin tocar hardware directo | interfaces |
| `AlarmController` | Orquestar el sistema (máquina de estados) | **interfaces**, nunca clases concretas |
| `main.cpp` | Crear instancias, **inyectarlas**, lanzar tareas FreeRTOS | todas |

> Regla de oro de dependencias: lo de arriba conoce interfaces, no implementaciones. `AlarmController` recibe `IGsm&`, `IAudioPlayer&`, `IStorage&` — así se puede testear con *mocks* y cambiar de SIM800L a SIM7600 sin tocar el controlador.

---

## Criterios de diseño
Plantillas de código completas en `references/patrones-firmware.md`.

Estos criterios están divididos en dos niveles. El nivel **Siempre** cubre restricciones técnicas reales del ESP32 donde desviarse provoca bugs silenciosos, crashes o condiciones de carrera. El nivel **Por defecto** son las mejores prácticas del proyecto — aplícalas salvo que haya una razón explícita y justificada para no hacerlo; si te desvías, documenta el porqué.

### Siempre (técnicamente obligatorio)

- **ISR mínimas.** Dentro de una ISR: **nada** de `Serial`, `delay()`, `malloc()` ni funciones pesadas. Solo marcar una bandera `volatile` o `xQueueSendFromISR`. Marcar `IRAM_ATTR`. El trabajo real va en la tarea. *Razón: las ISR corren en contexto privilegiado; una llamada bloqueante cuelga el sistema.*

- **Recursos compartidos con mutex.** Nunca dos tareas FreeRTOS escribiendo el mismo periférico (SD, Serial, UART) sin `SemaphoreHandle_t`. *Razón: corrupción de datos y crashes no reproducibles.*

- **Sin `delay()` en tareas reactivas.** En cualquier tarea que deba responder a eventos usar `vTaskDelay` o bloquear en una cola. `delay()` bloquea el scheduler de ese core. *Razón: el watchdog del ESP32 reinicia el chip si una tarea no cede CPU.*

- **Configuración centralizada en `Config.h`.** Pines, baudrates y timeouts en `namespace Config` con `constexpr`. *Razón: un número mágico suelto en tres archivos distintos garantiza que uno quede desactualizado.*

### Por defecto (aplicar salvo razón explícita)

- **Una clase, una responsabilidad (SRP).** Separa `Sim800Driver`, `Mp3Player`, `SdStorage`, `NotificationService`. Si necesitas la conjunción "y" para describir una clase, divídela. *Puede omitirse en componentes de prototipo muy pequeños y de vida corta.*

- **Diseño por eventos.** Las fuentes (botón ISR, SMS, REST, RF 433) publican en una cola FreeRTOS; un consumidor decide qué hacer. *Alternativa válida: polling periódico en un `loop()` simple cuando la latencia no importa y hay un único hilo.*

- **Interfaces + inyección de dependencias.** `AlarmController` recibe `IGsm&`, `IAudioPlayer&`, `IStorage&` por constructor. *Puede simplificarse a clases concretas en drivers que nunca se reemplazarán ni testearán de forma aislada.*

- **Máquina de estados explícita.** `enum class AlarmState` + `switch`. *Para flujos muy lineales (init secuencial, demo de 3 pasos) una secuencia simple es más legible.*

- **FreeRTOS por componente.** Una tarea por preocupación (red, GSM, audio, controlador, watchdog). *Para prototipos o demos de un solo flujo, un `loop()` con polling puede ser suficiente y más fácil de depurar.*

- **Logger estructurado.** `EventLogger` con niveles en lugar de `Serial.println` suelto. *En prototipado rápido, `Serial.println` está bien; introducir el logger antes de pasar a producción.*

- **`ErrorCode` en lugar de booleanos.** Cada `initialize()` devuelve `ErrorCode`. *Para scripts de init de una sola ejecución, un `bool` con log explícito puede ser suficiente.*

- **Lógica pura testeable en `env:native`.** La lógica sin dependencias de hardware corre en la PC. *Aplica donde exista lógica pura real (parseo, validación, transiciones de estado); no fuerces abstracciones solo para cubrir código trivial.*

---

## Convenciones de código
- **Nombres:** clases `PascalCase`, métodos/variables `camelCase`, constantes `Config::UPPER_CASE`, interfaces con prefijo `I` (`IGsm`).
- **Headers:** un `.h`/`.cpp` por clase; `#pragma once`; incluir lo mínimo; declarar dependencias por referencia/interfaz.
- **`constexpr` > `#define`** para constantes (tiene tipo y respeta scope).
- **`enum class` > `enum`** (no contamina el namespace, es fuertemente tipado).
- Funciones cortas; sin lógica de negocio dentro de drivers ni hardware dentro de servicios.
- Comentarios que explican el *porqué*, no el *qué* obvio.

---

## Máquina de estados de la alarma
Modelo base (ajustar a los tipos de evento de las bases):

```cpp
enum class AlarmState { IDLE, ARMED, TRIGGERED, PLAYING, NOTIFYING, WAITING_ACK, SILENCED, FAULT };
enum class EventType { BTN_PANIC, SMS_IN, REST_IN, RF433, ACK, SILENCE, TICK };
```

El `AlarmController` consume eventos de la cola y transiciona en un `switch(state)`. Cada transición registra con el logger y dispara acciones vía las interfaces (`audio.play(type)`, `gsm.sendSms(...)`). La tabla de transiciones y un ejemplo completo están en `references/patrones-firmware.md`.

---

## FreeRTOS en este proyecto
| Tarea | Hace | Se comunica por |
|-------|------|-----------------|
| `TaskController` | Corre la máquina de estados | recibe `eventQueue` |
| `TaskGsm` | AT del SIM800L, SMS entrante/saliente | publica en `eventQueue`, mutex de UART |
| `TaskAudio` | Comanda el DFPlayer | recibe orden del controlador |
| `TaskNet` | WiFi + servidor REST/SoftAP | publica en `eventQueue` |
| `TaskWatchdog` | Vigila que las tareas respondan | semáforo/heartbeat |

- ISR del botón → `xQueueSendFromISR(eventQueue, ...)`.
- SD y `Serial` (log) → protegidos con `SemaphoreHandle_t` (mutex).
- Crear con `xTaskCreatePinnedToCore`; el WiFi/BT conviene en el core 0, lógica en el core 1.

---

## Testing con PlatformIO
- Entorno `env:native` corre tests en la PC, sin ESP32 (framework Unity).
- Testea **lógica pura**: `validatePhoneNumber`, transiciones de la máquina de estados, parseo de SMS, formato de log.
- El hardware se aísla tras interfaces y se sustituye por *mocks* en el test.
- Comando: `pio test -e native`. Setup y ejemplo en `references/platformio-setup.md`.

---

## Git / control de versiones
- `.gitignore` debe incluir `.pio/`, `.vscode/` (salvo `extensions.json`), `*.bin`.
- Commits pequeños y descriptivos. Sugerencia de prefijos: `feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `chore:`.
- Una rama por funcionalidad (`feat/sms-trigger`), `main` siempre compilable.
- Nunca commitear credenciales WiFi ni números de teléfono reales: van en `Config.h` ignorado o en `secrets.h` fuera de Git.

---

## Flujo de trabajo al escribir o revisar código
1. **¿Demo/prototipo o código de producción?** En demo se puede simplificar la arquitectura; en producción se aplican todos los criterios "Por defecto". Decídelo antes de empezar.
2. **¿En qué capa va?** Hardware → driver; lógica reutilizable → service; orquestación → controller. Si no encaja en ninguna, repensar.
3. **¿Cumple los criterios "Siempre"?** ISR limpia, mutex en recursos compartidos, sin `delay()` bloqueante en tareas reactivas, Config.h. Estos no tienen excepción.
4. **¿Se desvía de algún criterio "Por defecto"?** Si sí, ¿hay una razón explícita? Documéntala en un comentario o en el commit.
5. **¿Bloquea cuando no debe?** `vTaskDelay` o bloqueo en cola en lugar de `delay()`.
6. Al terminar, pasa el **checklist de revisión**.

### Checklist de revisión (antes de dar por bueno un cambio)

**Siempre (bloquean el merge si fallan):**
- [ ] ISR sin `Serial`/`delay`/`malloc`, marcada `IRAM_ATTR`
- [ ] Recursos compartidos entre tareas protegidos con mutex
- [ ] Sin `delay()` bloqueante en tareas reactivas
- [ ] Sin números mágicos sueltos (pines, baudrates en `Config.h`)
- [ ] Compila sin errores en `esp32doit-devkit-v1`

**Por defecto (revisar y justificar si no aplican):**
- [ ] Clases con responsabilidad única; controlador depende de interfaces
- [ ] Errores manejados explícitamente (no ignorar el retorno de `begin()`)
- [ ] Logging estructurado antes de pasar a producción
- [ ] Lógica pura separada del hardware donde tenga sentido
- [ ] Tests en `env:native` si existe lógica pura testeable

---

## Referencias (empaquetadas dentro de esta skill)
Claude Code las abre cuando las necesita; no hay que manejarlas a mano.
- `references/platformio-setup.md` — `platformio.ini` completo, entornos (`esp32dev`/`native`), `lib_deps`, estructura de `lib/`, setup de tests Unity.
- `references/patrones-firmware.md` — Plantillas listas y adaptadas al proyecto: ISR+cola, máquina de estados con tabla de transiciones, inyección de dependencias, logger, manejo de errores, interfaces (`IGsm`/`IAudioPlayer`/`IStorage`).
