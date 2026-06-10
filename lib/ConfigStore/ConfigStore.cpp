#if defined(ARDUINO)
#include "ConfigStore.h"

void ConfigStore::begin() {
  mutex_ = xSemaphoreCreateMutex();
  prefs_.begin("alarma", false);   // namespace NVS en lectura/escritura
}

String ConfigStore::wifiSsid() {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  String v = prefs_.getString("ssid", "");
  xSemaphoreGive(mutex_);
  return v;
}

String ConfigStore::wifiPass() {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  String v = prefs_.getString("pass", "");
  xSemaphoreGive(mutex_);
  return v;
}

String ConfigStore::smsAllowList(const char* fallback) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  String v = prefs_.getString("sms", "");
  xSemaphoreGive(mutex_);
  return v.length() ? v : String(fallback);
}

void ConfigStore::setWifi(const String& ssid, const String& pass) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  prefs_.putString("ssid", ssid);
  prefs_.putString("pass", pass);
  xSemaphoreGive(mutex_);
}

void ConfigStore::setSmsAllowList(const String& list) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  prefs_.putString("sms", list);
  xSemaphoreGive(mutex_);
}

#endif // ARDUINO
