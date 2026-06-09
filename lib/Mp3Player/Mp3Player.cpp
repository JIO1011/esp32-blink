// Driver de hardware: sólo se compila para Arduino/ESP32 (regla del proyecto:
// los .cpp de HAL/drivers van bajo guard ARDUINO; en env:native se usa un fake).
#if defined(ARDUINO)
#include "Mp3Player.h"
#include "Logger.h"

Mp3Player::Mp3Player(HardwareSerial& uart, int8_t rxPin, int8_t txPin, uint32_t baud, uint8_t volume)
  : uart_(uart), rxPin_(rxPin), txPin_(txPin), baud_(baud), volume_(volume) {}

ErrorCode Mp3Player::initialize() {
  uart_.begin(baud_, SERIAL_8N1, rxPin_, txPin_);
  delay(1000);   // el DFPlayer necesita ~1-2 s tras energizarse antes del primer comando
  if (!df_.begin(uart_)) {
    Logger::error("DFPlayer no responde (revisa cableado, SD FAT32 con /mp3, alimentacion)");
    ready_ = false;
    return ErrorCode::AUDIO_ERROR;
  }
  df_.volume(volume_);   // rango 0..30
  ready_ = true;
  Logger::info("DFPlayer listo (vol=%u)", volume_);
  return ErrorCode::OK;
}

void Mp3Player::play(uint16_t track) {
  if (!ready_) return;
  df_.playMp3Folder(track);   // /mp3/000{track}.mp3
}

void Mp3Player::stop() {
  if (!ready_) return;
  df_.stop();
}

#endif // ARDUINO
