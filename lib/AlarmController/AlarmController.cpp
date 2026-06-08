#include "AlarmController.h"
#include "Logger.h"

AlarmController::AlarmController(IAlarmOutput& output) : output_(output) {}

bool AlarmController::isTrigger(EventType ev) {
  return ev == EventType::BTN_PANIC || ev == EventType::SMS_IN ||
         ev == EventType::REST_IN   || ev == EventType::RF433;
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
  // Fases 2-4 sumarán aquí: audio.play(trackFor(ev)), gsm.sendSms(...), storage.logEvent(...).
  output_.activate();
  state_.store(AlarmState::TRIGGERED, std::memory_order_relaxed);
  Logger::info("ALARMA ACTIVADA (origen=%s) -> TRIGGERED", nameOf(ev));
}

void AlarmController::silence() {
  output_.deactivate();
  state_.store(AlarmState::SILENCED, std::memory_order_relaxed);
  Logger::info("Alarma silenciada -> SILENCED");
}
