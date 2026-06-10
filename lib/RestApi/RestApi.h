#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <functional>
#include "AlarmController.h"
#include "SdStorage.h"
#include "Events.h"

// Web Service REST/JSON en la ESP32 (ANEXO 1: "operatividad como Web Service REST").
// Monta sus rutas en el WebServer COMPARTIDO con el portal (Fase 5). Cada request que
// opera la alarma se traduce a un EventType y entra en la MISMA cola del sistema
// (sin lógica duplicada). oAuth2.0/HTTPS quedan para backend/4G (Fase 7).
class RestApi {
public:
  using Publisher = std::function<void(EventType)>;   // encola el evento en el bus del sistema

  RestApi(WebServer& server, AlarmController& ctrl, SdStorage& storage, Publisher publish);
  void registerRoutes();

private:
  void handleEstado();      // GET  /api/estado    -> estado + red + RSSI + IP (JSON)
  void handleAlarma();      // POST /api/alarma    -> dispara (?tipo=panico|sospechoso|disuasivo)
  void handleSilenciar();   // POST /api/silenciar -> silencia
  void handleEventos();     // GET  /api/eventos   -> log CSV de la MicroSD

  WebServer&       server_;
  AlarmController& ctrl_;
  SdStorage&       storage_;
  Publisher        publish_;
};
