// Driver de hardware: sólo se compila para Arduino/ESP32 (en env:native se usa un fake).
#if defined(ARDUINO)
#include "SdStorage.h"
#include "Config.h"
#include "Logger.h"

SdStorage::SdStorage(uint8_t csPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin, const char* path)
  : csPin_(csPin), sckPin_(sckPin), misoPin_(misoPin), mosiPin_(mosiPin), path_(path) {}

// Intenta montar la SD varias veces a SPI lento (1 MHz): tolera cables largos/protoboard.
bool SdStorage::attemptMount() {
  for (int i = 0; i < 3; ++i) {
    if (SD.begin(csPin_, SPI, Config::SD_SPI_HZ)) {
      ready_ = true;
      return true;
    }
    SD.end();
    delay(120);
  }
  ready_ = false;
  return false;
}

// Remontaje en caliente con backoff: evita martillar SD.begin en cada evento si la SD está caída.
void SdStorage::ensureMountedLocked() {
  if (ready_) return;
  const uint32_t now = millis();
  if (now - lastMountMs_ < 5000) return;     // como mucho un reintento cada 5 s
  lastMountMs_ = now;
  if (attemptMount()) Logger::info("MicroSD remontada en caliente");
}

ErrorCode SdStorage::initialize() {
  mutex_ = xSemaphoreCreateMutex();
  if (mutex_ == nullptr) return ErrorCode::SD_ERROR;

  SPI.begin(sckPin_, misoPin_, mosiPin_, csPin_);
  if (!attemptMount()) {
    Logger::error("MicroSD no detectada (CS=%u); reintentara en caliente al registrar", csPin_);
    return ErrorCode::SD_ERROR;
  }
  if (!SD.exists(path_)) {                       // cabecera CSV la primera vez
    File f = SD.open(path_, FILE_WRITE);
    if (f) { f.println("timestamp_ms,origen,estado"); f.close(); }
  }
  Logger::info("MicroSD lista; log de eventos en %s", path_);
  return ErrorCode::OK;
}

ErrorCode SdStorage::logEvent(EventType origin, AlarmState state) {
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(200)) != pdTRUE) {
    Logger::warn("SD ocupada; evento no registrado");
    return ErrorCode::SD_ERROR;
  }
  // Ruta rápida: NO se intenta montar aquí (eso bloquearía el disparo de la alarma);
  // el remontaje vive en maintain() (tarea de fondo). Si la SD está caída, se omite el registro.
  ErrorCode rc = ErrorCode::OK;
  if (ready_) {
    File f = SD.open(path_, FILE_APPEND);
    if (f) {
      // Marca de tiempo relativa (millis). Producción: RTC DS3231 o NTP para fecha/hora real.
      f.printf("%lu,%s,%s\n", static_cast<unsigned long>(millis()), nameOf(origin), nameOf(state));
      f.close();
    } else {
      Logger::error("Escritura fallida en %s; se remontara en 2.º plano", path_);
      SD.end();
      ready_ = false;                             // maintain() la remontará
      rc = ErrorCode::SD_ERROR;
    }
  } else {
    rc = ErrorCode::SD_ERROR;
  }
  xSemaphoreGive(mutex_);
  return rc;
}

void SdStorage::maintain() {
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) != pdTRUE) return;
  ensureMountedLocked();   // intenta remontar con backoff sólo si está caída
  xSemaphoreGive(mutex_);
}

String SdStorage::readLog(size_t maxBytes) {
  String out;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(300)) != pdTRUE) return out;
  if (ready_) {
    File f = SD.open(path_, FILE_READ);
    if (f) {
      while (f.available() && out.length() < maxBytes) out += static_cast<char>(f.read());
      f.close();
    }
  }
  xSemaphoreGive(mutex_);
  return out;
}

void SdStorage::printLog() {
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(300)) != pdTRUE) return;
  if (ready_) {
    File f = SD.open(path_, FILE_READ);
    if (f) {
      Logger::info("---- contenido de %s ----", path_);
      while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.length()) Serial.printf("  %s\n", line.c_str());
      }
      Logger::info("---- fin del log ----");
      f.close();
    }
  }
  xSemaphoreGive(mutex_);
}

#endif // ARDUINO
