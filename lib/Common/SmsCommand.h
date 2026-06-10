#pragma once
#include "Events.h"
#include <string>

// Lógica PURA del manejo de SMS del SIM800L (sin Arduino) -> testeable en env:native.
// Separa el "qué decidir" (esto) del "cómo hablar con el módulo" (Sim800Driver).
namespace SmsCommand {

  // Extrae el remitente de una cabecera +CMT del SIM800L.
  // Ej.: +CMT: "+593991234567","","25/06/09,22:00:00+00"  -> sender = "+593991234567"
  bool parseCmtHeader(const std::string& line, std::string& sender);

  // Mapea el cuerpo del SMS a un evento de la alarma. Devuelve true y rellena 'ev'
  // si el texto es una orden reconocida (case-insensitive):
  //   disparo  : PANICO / EMERGENCIA / AUXILIO / ALARMA / SOS   -> EventType::SMS_IN
  //   silenciar: SILENC* / APAGAR / OK                          -> EventType::SILENCE
  bool toEvent(const std::string& body, EventType& ev);

  // ¿El remitente está en la allow-list? (lista separada por comas, formato internacional)
  bool isAllowedSender(const std::string& sender, const std::string& allowList);
}
