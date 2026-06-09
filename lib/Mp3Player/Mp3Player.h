#pragma once
#include "IAudioPlayer.h"
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

// Driver del DFPlayer Mini (TF-16P) detrás de IAudioPlayer.
// UART por hardware remapeada a los pines del DFPlayer, 9600 bps (criterio
// "Siempre": nada de SoftwareSerial). 1 kΩ en serie en el RX del módulo (hardware).
// Único escritor de esta UART -> sin mutex (lo usa sólo el controlador).
class Mp3Player : public IAudioPlayer {
public:
  Mp3Player(HardwareSerial& uart, int8_t rxPin, int8_t txPin, uint32_t baud, uint8_t volume = 25);

  ErrorCode initialize() override;     // arranca la UART y el módulo; llamar en setup()
  void play(uint16_t track) override;  // /mp3/000{track}.mp3
  void stop() override;

private:
  HardwareSerial&     uart_;
  int8_t              rxPin_;
  int8_t              txPin_;
  uint32_t            baud_;
  uint8_t             volume_;
  DFRobotDFPlayerMini df_;
  bool                ready_ = false;
};
