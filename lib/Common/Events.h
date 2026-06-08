#pragma once
#include <cstdint>

// Estados y eventos de la alarma. Tipos PUROS (sin Arduino) para que la máquina
// de estados (AlarmController) compile y se testee en env:native.
//
// La tabla de transiciones vive en AlarmController; aquí sólo el vocabulario.

enum class AlarmState : uint8_t {
  IDLE,         // sin armar
  ARMED,        // armada, a la espera de un disparo
  TRIGGERED,    // alarma activa (salida de potencia energizada)
  PLAYING,      // reproduciendo audio (Fase 2)
  NOTIFYING,    // notificando al exterior (Fases 4/7)
  WAITING_ACK,  // esperando confirmación del operador
  SILENCED,     // silenciada tras un disparo
  FAULT,        // fallo de hardware
};

enum class EventType : uint8_t {
  BTN_PANIC,    // pulsador físico (Fase 1)
  SMS_IN,       // SMS entrante autorizado (Fase 4)
  REST_IN,      // petición REST (Fase 6)
  RF433,        // botón RF 433 MHz (producción)
  ACK,          // confirmación del operador
  SILENCE,      // orden de silenciar
  TICK,         // pulso periódico (watchdog/temporización)
};

// Nombres legibles para logs y depuración. inline -> sin .cpp, sin ODR issues.
inline const char* nameOf(EventType ev) {
  switch (ev) {
    case EventType::BTN_PANIC: return "BTN_PANIC";
    case EventType::SMS_IN:    return "SMS_IN";
    case EventType::REST_IN:   return "REST_IN";
    case EventType::RF433:     return "RF433";
    case EventType::ACK:       return "ACK";
    case EventType::SILENCE:   return "SILENCE";
    case EventType::TICK:      return "TICK";
  }
  return "?";
}

inline const char* nameOf(AlarmState st) {
  switch (st) {
    case AlarmState::IDLE:        return "IDLE";
    case AlarmState::ARMED:       return "ARMED";
    case AlarmState::TRIGGERED:   return "TRIGGERED";
    case AlarmState::PLAYING:     return "PLAYING";
    case AlarmState::NOTIFYING:   return "NOTIFYING";
    case AlarmState::WAITING_ACK: return "WAITING_ACK";
    case AlarmState::SILENCED:    return "SILENCED";
    case AlarmState::FAULT:       return "FAULT";
  }
  return "?";
}
