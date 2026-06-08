# Patrones de firmware — plantillas del proyecto

Plantillas listas para copiar y adaptar. Todas siguen las 10 reglas del SKILL.md.

## 1. Config centralizada — include/Config.h
```cpp
#pragma once
#include <cstdint>

namespace Config {
  // Pines (ver pinout en la skill de hardware)
  constexpr uint8_t PIN_SIM_RX   = 26;   // SIM800L TX -> ESP32
  constexpr uint8_t PIN_SIM_TX   = 27;   // ESP32 -> SIM800L RX (con divisor)
  constexpr uint8_t PIN_MP3_RX   = 16;
  constexpr uint8_t PIN_MP3_TX   = 17;
  constexpr uint8_t PIN_SD_CS    = 5;
  constexpr uint8_t PIN_RELAY    = 13;
  constexpr uint8_t PIN_LED      = 2;

  constexpr uint32_t GSM_BAUD    = 9600;
  constexpr uint32_t MP3_BAUD    = 9600;
  constexpr uint32_t LOG_BAUD    = 115200;

  // Mapa de audios (carpeta /mp3 del DFPlayer)
  constexpr uint16_t TRACK_PANIC      = 1;
  constexpr uint16_t TRACK_SUSPICIOUS = 2;
  constexpr uint16_t TRACK_DETERRENT  = 3;
}
```

## 2. Interfaces (contratos) para inyección y testing
```cpp
// lib/Sim800Driver/IGsm.h
#pragma once
#include "ErrorCode.h"
#include <Arduino.h>

class IGsm {
public:
  virtual ~IGsm() = default;
  virtual ErrorCode initialize() = 0;
  virtual ErrorCode sendSms(const String& to, const String& body) = 0;
  virtual bool       pollIncoming(String& from, String& body) = 0;
};
```
```cpp
// lib/Mp3Player/IAudioPlayer.h
#pragma once
#include "ErrorCode.h"
class IAudioPlayer {
public:
  virtual ~IAudioPlayer() = default;
  virtual ErrorCode initialize() = 0;
  virtual void play(uint16_t track) = 0;
  virtual void stop() = 0;
};
```

## 3. ErrorCode — lib/Common/ErrorCode.h
```cpp
#pragma once
enum class ErrorCode { OK, SD_ERROR, GSM_ERROR, AUDIO_ERROR, CONFIG_ERROR };
```

## 4. Logger con niveles — lib/EventLogger
```cpp
#pragma once
#include <Arduino.h>
namespace Logger {
  inline void info (const String& m){ Serial.printf("[INFO] %s\n",  m.c_str()); }
  inline void warn (const String& m){ Serial.printf("[WARN] %s\n",  m.c_str()); }
  inline void error(const String& m){ Serial.printf("[ERROR] %s\n", m.c_str()); }
}
// Producción: proteger Serial con mutex y/o volcar también a la microSD.
```

## 5. ISR + cola FreeRTOS (botón de pánico)
```cpp
QueueHandle_t eventQueue;   // creada en setup: xQueueCreate(10, sizeof(EventType));

void IRAM_ATTR buttonISR() {
  EventType ev = EventType::BTN_PANIC;
  BaseType_t hpw = pdFALSE;
  xQueueSendFromISR(eventQueue, &ev, &hpw);
  if (hpw) portYIELD_FROM_ISR();
}
// setup: attachInterrupt(Config::PIN_BTN, buttonISR, FALLING);
```

## 6. Máquina de estados — AlarmController
Tabla de transiciones (evento × estado → acción/estado):

| Estado \ Evento | BTN_PANIC / SMS_IN / REST_IN | SILENCE | ACK |
|-----------------|------------------------------|---------|-----|
| IDLE/ARMED      | → TRIGGERED (play + notify)  | —       | —   |
| TRIGGERED       | (reentra)                    | → SILENCED | → WAITING_ACK |
| SILENCED        | → TRIGGERED                  | —       | —   |

```cpp
class AlarmController {
public:
  AlarmController(IGsm& gsm, IAudioPlayer& audio, IStorage& storage)
    : gsm_(gsm), audio_(audio), storage_(storage) {}

  void onEvent(EventType ev) {
    switch (state_) {
      case AlarmState::IDLE:
      case AlarmState::ARMED:
        if (isTrigger(ev)) trigger(ev);
        break;
      case AlarmState::TRIGGERED:
        if (ev == EventType::SILENCE) { audio_.stop(); state_ = AlarmState::SILENCED; }
        break;
      case AlarmState::SILENCED:
        if (isTrigger(ev)) trigger(ev);
        break;
      default: break;
    }
  }
private:
  void trigger(EventType ev) {
    uint16_t track = trackFor(ev);
    audio_.play(track);
    gsm_.sendSms(Config::ADMIN_PHONE, "Alarma activada");
    storage_.logEvent(ev);
    Logger::info("Estado -> TRIGGERED");
    state_ = AlarmState::TRIGGERED;
  }
  static bool isTrigger(EventType e){ return e==EventType::BTN_PANIC||e==EventType::SMS_IN||e==EventType::REST_IN; }
  IGsm& gsm_; IAudioPlayer& audio_; IStorage& storage_;
  AlarmState state_ = AlarmState::ARMED;
};
```

## 7. main.cpp — composición e inyección
```cpp
#include <Arduino.h>
#include "Config.h"
#include "Sim800Driver.h"
#include "Mp3Player.h"
#include "SdStorage.h"
#include "AlarmController.h"

Sim800Driver gsm;        // implementan IGsm / IAudioPlayer / IStorage
Mp3Player    audio;
SdStorage    storage;
AlarmController* controller;   // recibe las dependencias por constructor

QueueHandle_t eventQueue;

void TaskController(void*) {
  EventType ev;
  for (;;) if (xQueueReceive(eventQueue, &ev, portMAX_DELAY)) controller->onEvent(ev);
}

void setup() {
  Serial.begin(Config::LOG_BAUD);
  if (gsm.initialize()     != ErrorCode::OK) Logger::error("GSM init");
  if (audio.initialize()   != ErrorCode::OK) Logger::error("Audio init");
  if (storage.initialize() != ErrorCode::OK) Logger::error("SD init");

  controller = new AlarmController(gsm, audio, storage);   // inyección
  eventQueue = xQueueCreate(10, sizeof(EventType));
  xTaskCreatePinnedToCore(TaskController, "ctrl", 4096, nullptr, 1, nullptr, 1);
  // ... attachInterrupt del botón, TaskGsm, TaskNet, etc.
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }   // el trabajo vive en las tareas
```

## 8. Mutex para recurso compartido (SD / Serial)
```cpp
SemaphoreHandle_t sdMutex = xSemaphoreCreateMutex();

void writeLog(const String& line) {
  if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    // ... escribir en la microSD ...
    xSemaphoreGive(sdMutex);
  } else {
    Logger::warn("SD ocupada, log descartado");
  }
}
```

## 9. Lógica pura testeable — PhoneUtils
```cpp
// lib/Common/PhoneUtils.h
#pragma once
#include <Arduino.h>
inline bool validatePhoneNumber(const String& p) {
  if (!p.startsWith("+")) return false;
  if (p.length() < 11 || p.length() > 15) return false;
  for (size_t i = 1; i < p.length(); ++i) if (!isDigit(p[i])) return false;
  return true;
}
```
Esta función no toca hardware → se prueba en `env:native` (ver `platformio-setup.md`).
