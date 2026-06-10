#pragma once
#include <atomic>
#include "Events.h"
#include "IAlarmOutput.h"
#include "IAudioPlayer.h"
#include "IStorage.h"

// Máquina de estados de la alarma. Orquesta el sistema a partir de eventos que
// llegan por la cola FreeRTOS (botón, SMS, REST...). Lógica PURA: no toca el
// hardware directamente, sólo interfaces -> testeable en env:native.
//
// Tabla de transiciones (Fases 1-3; se ampliará por fase):
//   IDLE/ARMED/SILENCED  + trigger  -> TRIGGERED (activa salida + audio + log)
//   TRIGGERED            + SILENCE   -> SILENCED  (desactiva + para audio + log)
//   TRIGGERED            + trigger   -> TRIGGERED (idempotente, ya activa)
class AlarmController {
public:
  AlarmController(IAlarmOutput& output, IAudioPlayer& audio, IStorage& storage);

  // Procesa un evento y transiciona. Lo invoca TaskController desde la cola.
  void onEvent(EventType ev);

  // Estado actual. Atómico: lo ESCRIBE TaskController y lo LEE TaskStatusLed;
  // un enum es recurso compartido entre tareas (criterio "Siempre").
  AlarmState state() const { return state_.load(std::memory_order_relaxed); }

private:
  void trigger(EventType ev);
  void silence();
  static bool     isTrigger(EventType ev);
  static uint16_t trackFor(EventType ev);   // mapea el evento a la pista del DFPlayer

  IAlarmOutput& output_;
  IAudioPlayer& audio_;
  IStorage&     storage_;
  std::atomic<AlarmState> state_{AlarmState::ARMED};
};
