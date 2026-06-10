#include "AlarmController.h"
#include "Config.h"
#include "Logger.h"

AlarmController::AlarmController(IAlarmOutput& output, IAudioPlayer& audio, IStorage& storage)
  : output_(output), audio_(audio), storage_(storage) {}

bool AlarmController::isTrigger(EventType ev) {
  return ev == EventType::BTN_PANIC || ev == EventType::SMS_IN ||
         ev == EventType::REST_IN   || ev == EventType::RF433;
}

// Mapa evento -> pista (carpeta /mp3 del DFPlayer). En Fase 2 el "tipo de alarma"
// se deriva de la FUENTE (placeholder para demostrar "audio por tipo" y ">=2 audios").
// En Fase 4 el tipo vendrá del TEXTO del SMS y en Fase 6 del cuerpo de la petición REST.
uint16_t AlarmController::trackFor(EventType ev) {
  switch (ev) {
    case EventType::SMS_IN:  return Config::TRACK_SUSPICIOUS;
    case EventType::REST_IN: return Config::TRACK_DETERRENT;
    default:                 return Config::TRACK_PANIC;   // BTN_PANIC, RF433
  }
}

void AlarmController::onEvent(EventType ev) {
  switch (state_.load(std::memory_order_relaxed)) {
    case AlarmState::IDLE:
    case AlarmState::ARMED:
    case AlarmState::SILENCED:
      if (isTrigger(ev)) trigger(ev);
      break;

    case AlarmState::TRIGGERED:
      if (ev == EventType::SILENCE) silence();
      // Otros disparos mientras ya está activa: se ignoran (idempotente).
      break;

    default:
      break;
  }
}

void AlarmController::trigger(EventType ev) {
  const uint16_t track = trackFor(ev);
  output_.activate();
  audio_.play(track);
  storage_.logEvent(ev, AlarmState::TRIGGERED);   // auditoría local (MicroSD)
  state_.store(AlarmState::TRIGGERED, std::memory_order_relaxed);
  Logger::info("ALARMA ACTIVADA (origen=%s, pista=%u) -> TRIGGERED", nameOf(ev), track);
}

void AlarmController::silence() {
  output_.deactivate();
  audio_.stop();
  storage_.logEvent(EventType::SILENCE, AlarmState::SILENCED);
  state_.store(AlarmState::SILENCED, std::memory_order_relaxed);
  Logger::info("Alarma silenciada -> SILENCED");
}
