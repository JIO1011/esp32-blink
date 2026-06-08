#include "Logger.h"
#include <cstdarg>
#include <cstdio>

#if defined(ARDUINO)
  #include <Arduino.h>
#endif

namespace {
  // Formatea una sola vez y emite con prefijo de nivel.
  void emit(const char* level, const char* fmt, va_list ap) {
    char buf[160];
    vsnprintf(buf, sizeof(buf), fmt, ap);
#if defined(ARDUINO)
    Serial.printf("[%s] %s\n", level, buf);
#else
    std::printf("[%s] %s\n", level, buf);
#endif
  }
}

namespace Logger {
  void info(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); emit("INFO",  fmt, ap); va_end(ap); }
  void warn(const char* fmt, ...)  { va_list ap; va_start(ap, fmt); emit("WARN",  fmt, ap); va_end(ap); }
  void error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); emit("ERROR", fmt, ap); va_end(ap); }
}
