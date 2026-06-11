#if defined(ARDUINO)
#include "NetPortal.h"
#include "Logger.h"

NetPortal::NetPortal(ConfigStore& cfg, const char* apSsid, const char* apPass, WebServer& server)
  : cfg_(cfg), apSsid_(apSsid), apPass_(apPass), server_(server) {}

void NetPortal::begin() {
  WiFi.mode(WIFI_AP_STA);                 // SoftAP + Station simultáneos
  WiFi.softAP(apSsid_, apPass_);
  Logger::info("SoftAP '%s' activo en %s", apSsid_, WiFi.softAPIP().toString().c_str());
  connectStation();

  server_.on("/",     HTTP_GET,  [this]() { handleRoot(); });
  server_.on("/save", HTTP_POST, [this]() { handleSave(); });
  server_.begin();
  Logger::info("Portal de config en http://%s/", WiFi.softAPIP().toString().c_str());
}

void NetPortal::handle() {
  server_.handleClient();
}

void NetPortal::connectStation() {
  const String ssid = cfg_.wifiSsid();
  if (ssid.isEmpty()) {
    Logger::warn("Sin SSID configurado; solo SoftAP (entra al portal para configurar)");
    return;
  }
  WiFi.begin(ssid.c_str(), cfg_.wifiPass().c_str());   // no bloqueante
  Logger::info("Conectando a WiFi '%s'...", ssid.c_str());
}

void NetPortal::handleRoot() {
  const String ssid = cfg_.wifiSsid();
  const String sms  = cfg_.smsAllowList("");
  const bool   sta  = WiFi.status() == WL_CONNECTED;

  String h = "<!DOCTYPE html><html><head><meta name=viewport "
             "content='width=device-width,initial-scale=1'><title>Central - Config</title></head>"
             "<body style='font-family:sans-serif;max-width:420px;margin:20px auto;padding:0 12px'>";
  h += "<h2>Configuracion de la Central</h2>";
  h += "<form method='POST' action='/save'>";
  h += "WiFi SSID:<br><input name='ssid' value='" + ssid + "' style='width:100%'><br><br>";
  h += "WiFi clave (vacio = mantener):<br><input name='pass' type='password' style='width:100%'><br><br>";
  h += "Numeros SMS autorizados (separa con coma):<br><input name='sms' value='" + sms + "' style='width:100%'><br><br>";
  h += "<button type='submit'>Guardar</button></form><hr>";
  h += "<p>STA: " + String(sta ? "conectado, IP " + WiFi.localIP().toString() + ", RSSI " + String(WiFi.RSSI()) + " dBm"
                              : "no conectado") + "</p>";
  h += "</body></html>";
  server_.send(200, "text/html", h);
}

void NetPortal::handleSave() {
  const String ssid = server_.arg("ssid");
  String pass = server_.arg("pass");
  const String sms = server_.arg("sms");

  if (ssid.length()) {
    if (pass.isEmpty()) pass = cfg_.wifiPass();   // vacío = conservar la clave anterior
    cfg_.setWifi(ssid, pass);
  }
  if (sms.length()) cfg_.setSmsAllowList(sms);    // vacío = conservar la lista (no obligatorio)

  server_.send(200, "text/html",
    "<p>Guardado. Reconectando WiFi...</p><a href='/'>Volver</a>");
  Logger::info("Config guardada por portal; reconectando STA a '%s'", ssid.c_str());
  connectStation();
}

#endif // ARDUINO
