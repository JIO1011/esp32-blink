#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Persistencia de configuración en NVS (Preferences). Permite cambiar credenciales
// y parámetros SIN recompilar (config inalámbrica del ANEXO 1) y conservarlos tras
// reinicio. Las credenciales reales viven aquí (NVS), nunca en el código/git.
//
// Lo usan TaskNet (escribe desde el portal) y TaskGsm (lee la allow-list) -> mutex.
class ConfigStore {
public:
  void   begin();
  String wifiSsid();
  String wifiPass();
  String smsAllowList(const char* fallback);   // lo guardado, o 'fallback' si está vacío
  void   setWifi(const String& ssid, const String& pass);
  void   setSmsAllowList(const String& list);

private:
  Preferences       prefs_;
  SemaphoreHandle_t mutex_ = nullptr;
};
