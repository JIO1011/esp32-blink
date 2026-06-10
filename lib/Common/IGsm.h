#pragma once
#include "ErrorCode.h"
#include <string>

// Contrato (puerto) del módulo GSM (SIM800L). Permite recibir SMS para activar la
// central sin internet (diferenciador del ANEXO 1 §4) y enviar SMS de notificación.
//
// Usa std::string (no Arduino String) para que la lógica que lo rodea sea
// portable/testeable; el driver concreto convierte en su frontera.
//
// En Fase 4 lo consume TaskGsm (fuente de eventos), no el AlarmController:
// un SMS válido se traduce a EventType y se publica en la misma eventQueue.
class IGsm {
public:
  virtual ~IGsm() = default;

  virtual ErrorCode initialize() = 0;

  // Si llegó un SMS (push +CMT), devuelve true y rellena remitente y cuerpo.
  // No bloqueante: devuelve false si no hay nada pendiente.
  virtual bool pollIncoming(std::string& from, std::string& body) = 0;

  // Envía un SMS (notificación). Se usará a fondo en notificaciones/Fase 7.
  virtual ErrorCode sendSms(const std::string& to, const std::string& body) = 0;
};
