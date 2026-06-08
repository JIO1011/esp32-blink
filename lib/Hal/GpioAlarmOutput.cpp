// Sólo se compila para el target Arduino/ESP32. En env:native este driver no se
// incluye (el test usa un fake), y el guard evita que un escaneo del LDF lo rompa.
#if defined(ARDUINO)
#include "GpioAlarmOutput.h"
#include <Arduino.h>

void GpioAlarmOutput::begin() {
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);   // arranca desactivada
}

void GpioAlarmOutput::activate()   { digitalWrite(pin_, HIGH); }
void GpioAlarmOutput::deactivate() { digitalWrite(pin_, LOW); }

#endif // ARDUINO
