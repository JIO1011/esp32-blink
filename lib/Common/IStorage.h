#pragma once
#include "ErrorCode.h"
#include "Events.h"

// Contrato (puerto) del almacenamiento de eventos del ESP32: log/auditoría local
// en la MicroSD. Es la base local del "respaldo automático de BD" del ANEXO 1
// (la parte que vive en el dispositivo).
//
// La marca de tiempo la pone la IMPLEMENTACIÓN (millis() en demo; RTC/NTP en
// producción), de modo que AlarmController siga siendo lógica pura testeable en
// env:native sin depender del reloj ni del hardware.
class IStorage {
public:
  virtual ~IStorage() = default;
  virtual ErrorCode initialize() = 0;
  virtual ErrorCode logEvent(EventType origin, AlarmState state) = 0;
};
