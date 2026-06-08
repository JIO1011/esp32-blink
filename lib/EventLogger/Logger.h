#pragma once

// Logger estructurado con niveles (criterio "Por defecto" de la skill de
// firmware). Acepta formato printf. Implementación native-safe: en el ESP32
// escribe por Serial; en env:native escribe por stdout (para que la lógica que
// loguea siga compilando en los tests).
//
// Producción: proteger la salida con mutex y/o volcar también a la microSD.
namespace Logger {
  void info (const char* fmt, ...);
  void warn (const char* fmt, ...);
  void error(const char* fmt, ...);
}
