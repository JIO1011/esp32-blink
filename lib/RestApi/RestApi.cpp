#if defined(ARDUINO)
#include "RestApi.h"
#include <WiFi.h>
#include "Logger.h"

RestApi::RestApi(WebServer& server, AlarmController& ctrl, SdStorage& storage, Publisher publish)
  : server_(server), ctrl_(ctrl), storage_(storage), publish_(publish) {}

void RestApi::registerRoutes() {
  server_.on("/api/estado",    HTTP_GET,  [this] { handleEstado(); });
  server_.on("/api/alarma",    HTTP_POST, [this] { handleAlarma(); });
  server_.on("/api/silenciar", HTTP_POST, [this] { handleSilenciar(); });
  server_.on("/api/eventos",   HTTP_GET,  [this] { handleEventos(); });
  Logger::info("REST API: GET /api/estado /api/eventos | POST /api/alarma /api/silenciar");
}

void RestApi::handleEstado() {
  const bool sta = WiFi.status() == WL_CONNECTED;
  String json = "{";
  json += "\"estado\":\"" + String(nameOf(ctrl_.state())) + "\",";
  json += "\"red\":\""    + String(sta ? "WiFi-STA" : "SoftAP") + "\",";
  json += "\"rssi\":"     + String(sta ? WiFi.RSSI() : 0) + ",";
  json += "\"ip\":\""     + (sta ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\"";
  json += "}";
  server_.send(200, "application/json", json);
}

void RestApi::handleAlarma() {
  // ?tipo=panico|sospechoso|disuasivo (opcional). Por defecto: REST_IN (disuasivo).
  EventType ev = EventType::REST_IN;
  const String t = server_.arg("tipo");
  if      (t == "panico")     ev = EventType::BTN_PANIC;
  else if (t == "sospechoso") ev = EventType::SMS_IN;
  else if (t == "disuasivo")  ev = EventType::REST_IN;
  publish_(ev);
  server_.send(200, "application/json",
               String("{\"ok\":true,\"evento\":\"") + nameOf(ev) + "\"}");
}

void RestApi::handleSilenciar() {
  publish_(EventType::SILENCE);
  server_.send(200, "application/json", "{\"ok\":true,\"evento\":\"SILENCE\"}");
}

void RestApi::handleEventos() {
  String csv = storage_.readLog(4096);
  if (csv.isEmpty()) csv = "(sin registros o MicroSD no disponible)\n";
  server_.send(200, "text/csv", csv);
}

#endif // ARDUINO
