#pragma once
#include "IAlarmOutput.h"
#include <cstdint>

// Implementación de IAlarmOutput sobre un GPIO del ESP32 (activo en alto).
// Placeholder de la etapa de potencia de producción (2 relés de estado sólido).
class GpioAlarmOutput : public IAlarmOutput {
public:
  explicit GpioAlarmOutput(uint8_t pin) : pin_(pin) {}

  void begin();                  // configura el pin; llamar en setup()
  void activate()   override;
  void deactivate() override;

private:
  uint8_t pin_;
};
