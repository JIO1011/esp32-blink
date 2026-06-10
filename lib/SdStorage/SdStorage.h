#pragma once
#include "IStorage.h"
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Driver de la MicroSD del ESP32 (bus VSPI) detrás de IStorage. Registra cada
// evento como una línea CSV. Es una SD INDEPENDIENTE de la del DFPlayer (zócalos
// distintos: el DFPlayer lee audios, el ESP32 registra eventos).
//
// Protege la SD con un MUTEX (criterio "Siempre": recurso compartido entre
// tareas; ninguna escribe sin tomar el semáforo).
class SdStorage : public IStorage {
public:
  SdStorage(uint8_t csPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin, const char* path);

  ErrorCode initialize() override;
  ErrorCode logEvent(EventType origin, AlarmState state) override;
  void printLog();   // vuelca el log por Serial (demo: llamar en setup, hilo único)

private:
  uint8_t  csPin_;
  int8_t   sckPin_;
  int8_t   misoPin_;
  int8_t   mosiPin_;
  const char* path_;
  SemaphoreHandle_t mutex_ = nullptr;
  bool ready_ = false;
};
