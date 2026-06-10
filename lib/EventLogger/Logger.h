#pragma once

// Logger estructurado con niveles (criterio "Por defecto" de la skill de firmware).
// Acepta formato printf. Native-safe: en el ESP32 escribe por Serial protegido con
// un mutex; en env:native escribe por stdout (para que la lógica que loguea compile
// en los tests).
//
// Desde la Fase 4 hay varias tareas escribiendo Serial (TaskController, TaskGsm...),
// así que el acceso va protegido por mutex (criterio "Siempre": recurso compartido).
namespace Logger {
  void begin();   // crea el mutex de Serial (Arduino). No-op en native. Llamar al inicio de setup().
  void info (const char* fmt, ...);
  void warn (const char* fmt, ...);
  void error(const char* fmt, ...);
}
