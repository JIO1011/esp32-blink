#pragma once
#include "ErrorCode.h"
#include <cstdint>

// Contrato (puerto) del reproductor de audio por tipo de evento (Fase 2).
// Implementación concreta: Mp3Player sobre DFPlayer Mini. AlarmController sólo
// conoce esta interfaz -> testeable en env:native con un fake.
class IAudioPlayer {
public:
  virtual ~IAudioPlayer() = default;
  virtual ErrorCode initialize() = 0;
  virtual void play(uint16_t track) = 0;  // reproduce /mp3/000{track}.mp3
  virtual void stop() = 0;
};
