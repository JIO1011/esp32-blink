// Driver de hardware: sólo se compila para Arduino/ESP32 (en env:native se usa un fake).
#if defined(ARDUINO)
#include "SdStorage.h"
#include "Logger.h"

SdStorage::SdStorage(uint8_t csPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin, const char* path)
  : csPin_(csPin), sckPin_(sckPin), misoPin_(misoPin), mosiPin_(mosiPin), path_(path) {}

ErrorCode SdStorage::initialize() {
  mutex_ = xSemaphoreCreateMutex();
  if (mutex_ == nullptr) return ErrorCode::SD_ERROR;

  SPI.begin(sckPin_, misoPin_, mosiPin_, csPin_);
  if (!SD.begin(csPin_)) {
    Logger::error("MicroSD no detectada (CS=%u); revisa cableado VSPI y formato FAT32", csPin_);
    return ErrorCode::SD_ERROR;
  }
  if (!SD.exists(path_)) {                       // cabecera CSV la primera vez
    File f = SD.open(path_, FILE_WRITE);
    if (f) { f.println("timestamp_ms,origen,estado"); f.close(); }
  }
  ready_ = true;
  Logger::info("MicroSD lista; log de eventos en %s", path_);
  return ErrorCode::OK;
}

ErrorCode SdStorage::logEvent(EventType origin, AlarmState state) {
  if (!ready_) return ErrorCode::SD_ERROR;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(200)) != pdTRUE) {
    Logger::warn("SD ocupada; evento no registrado");
    return ErrorCode::SD_ERROR;
  }
  ErrorCode rc = ErrorCode::OK;
  File f = SD.open(path_, FILE_APPEND);
  if (f) {
    // Marca de tiempo relativa (millis). Producción: RTC DS3231 o NTP para fecha/hora real.
    f.printf("%lu,%s,%s\n", (unsigned long)millis(), nameOf(origin), nameOf(state));
    f.close();
  } else {
    Logger::error("No se pudo abrir %s para registrar", path_);
    rc = ErrorCode::SD_ERROR;
  }
  xSemaphoreGive(mutex_);
  return rc;
}

void SdStorage::printLog() {
  if (!ready_) return;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(300)) != pdTRUE) return;
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
  xSemaphoreGive(mutex_);
}

#endif // ARDUINO
