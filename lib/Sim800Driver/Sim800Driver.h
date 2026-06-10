#pragma once
#include "IGsm.h"
#include <Arduino.h>

// Driver del SIM800L detrás de IGsm, por AT crudo sobre una UART HW (UART2).
// Se usa AT directo (no TinyGSM) porque la RECEPCIÓN de SMS por push CNMI (+CMT:)
// es más simple y robusta así; TinyGSM se reserva para GPRS/HTTP en la Fase 7.
//
// ⚠️ HARDWARE (skill alarmas-comunitarias-hw): el SIM800L exige rail dedicado
// ~4.0 V a >=2 A + condensador 1500 µF; NUNCA del 3V3 del ESP32 (causa #1 de fallos).
// ESP32 TX(27) -> SIM RX vía divisor (~2.8 V); SIM TX -> ESP32 RX(26) directo.
class Sim800Driver : public IGsm {
public:
  Sim800Driver(HardwareSerial& uart, int8_t rxPin, int8_t txPin, uint32_t baud, const char* apn);

  ErrorCode initialize() override;
  bool      pollIncoming(std::string& from, std::string& body) override;
  ErrorCode sendSms(const std::string& to, const std::string& body) override;
  ErrorCode httpPostJson(const std::string& url, const std::string& json) override;

private:
  bool sendAt(const char* cmd, const char* expect, uint32_t timeoutMs);

  HardwareSerial& uart_;
  int8_t      rxPin_;
  int8_t      txPin_;
  uint32_t    baud_;
  const char* apn_;
  bool        ready_ = false;
};
