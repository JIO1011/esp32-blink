#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigStore.h"

// WiFi en Station + SoftAP simultáneo + portal web de configuración servido por la
// ESP32. Cumple "configurable de forma inalámbrica en cualquier momento, sin
// botones ni jumpers" (ANEXO 1). Guarda en NVS vía ConfigStore.
class NetPortal {
public:
  // El WebServer se inyecta (compartido) para que la API REST (Fase 6) monte sus
  // rutas en el mismo servidor del puerto 80.
  NetPortal(ConfigStore& cfg, const char* apSsid, const char* apPass, WebServer& server);
  void begin();    // levanta AP+STA, conecta a STA si hay credenciales, arranca el WebServer
  void handle();   // server.handleClient(); llamar periódicamente en TaskNet

private:
  void connectStation();
  void handleRoot();
  void handleSave();

  ConfigStore& cfg_;
  const char*  apSsid_;
  const char*  apPass_;
  WebServer&   server_;
};
