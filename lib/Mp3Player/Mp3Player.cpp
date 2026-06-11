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

  // 1) Init normal (reset + ACK), con reintentos: módulos genuinos / arranque lento.
  for (int i = 0; i < 3 && !ready_; ++i) {
    if (df_.begin(uart_, /*isACK=*/true, /*doReset=*/true)) ready_ = true;
    else delay(800);
  }
  if (ready_) {
    df_.volume(volume_);   // rango 0..30
    Logger::info("DFPlayer listo (vol=%u)", volume_);
    return ErrorCode::OK;
  }

  // 2) Fallback "clon": muchos DFPlayer clones (GD3200B/MH2024K...) no responden a la
  //    consulta de estado pero SÍ reproducen. begin(sin reset/ACK) los acepta para mandar comandos.
  df_.begin(uart_, /*isACK=*/false, /*doReset=*/false);
  df_.volume(volume_);
  ready_ = true;
  Logger::warn("DFPlayer sin ACK (posible clon o 5V flojo): intentara reproducir igual");
  Logger::warn("  si NO suena -> 5V estables + GND comun + 1k en RX + SD FAT32 con /mp3/0001.mp3");
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
