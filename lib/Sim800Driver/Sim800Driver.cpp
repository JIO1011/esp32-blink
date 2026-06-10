// Driver de hardware: sólo se compila para Arduino/ESP32 (en env:native se usa un fake/mocks).
#if defined(ARDUINO)
#include "Sim800Driver.h"
#include "SmsCommand.h"
#include "Logger.h"

Sim800Driver::Sim800Driver(HardwareSerial& uart, int8_t rxPin, int8_t txPin, uint32_t baud)
  : uart_(uart), rxPin_(rxPin), txPin_(txPin), baud_(baud) {}

bool Sim800Driver::sendAt(const char* cmd, const char* expect, uint32_t timeoutMs) {
  while (uart_.available()) uart_.read();          // limpia residuos
  uart_.printf("%s\r\n", cmd);
  const uint32_t t0 = millis();
  String resp;
  while (millis() - t0 < timeoutMs) {
    while (uart_.available()) resp += static_cast<char>(uart_.read());
    if (resp.indexOf(expect) >= 0) return true;
    if (resp.indexOf("ERROR") >= 0) return false;
    delay(5);
  }
  return false;
}

ErrorCode Sim800Driver::initialize() {
  uart_.begin(baud_, SERIAL_8N1, rxPin_, txPin_);
  delay(3000);                                     // el SIM800L tarda en arrancar/registrar
  if (!sendAt("AT", "OK", 1000)) {
    Logger::error("SIM800L no responde a AT (alimentacion/antena/cableado)");
    return ErrorCode::GSM_ERROR;
  }
  sendAt("ATE0", "OK", 1000);                      // sin eco
  sendAt("AT+CMGF=1", "OK", 1000);                 // SMS en modo texto
  sendAt("AT+CNMI=2,2,0,0,0", "OK", 1000);         // push de SMS entrante al serial (+CMT:)
  ready_ = true;
  Logger::info("SIM800L listo (SMS modo texto + push CNMI)");
  return ErrorCode::OK;
}

bool Sim800Driver::pollIncoming(std::string& from, std::string& body) {
  if (!ready_ || !uart_.available()) return false;

  String line = uart_.readStringUntil('\n');
  line.trim();
  if (!line.startsWith("+CMT:")) return false;     // sólo nos interesan los pushes de SMS

  std::string sender;
  if (!SmsCommand::parseCmtHeader(line.c_str(), sender)) return false;

  // El cuerpo llega en la línea siguiente; espera breve a que aparezca.
  const uint32_t t0 = millis();
  while (!uart_.available() && millis() - t0 < 300) delay(2);
  String b = uart_.readStringUntil('\n');
  b.trim();

  from = sender;
  body = b.c_str();
  return true;
}

ErrorCode Sim800Driver::sendSms(const std::string& to, const std::string& body) {
  if (!ready_) return ErrorCode::GSM_ERROR;
  uart_.printf("AT+CMGS=\"%s\"\r", to.c_str());
  delay(200);                                      // espera el prompt '>'
  uart_.print(body.c_str());
  uart_.write(26);                                 // Ctrl-Z: enviar
  return sendAt("", "OK", 8000) ? ErrorCode::OK : ErrorCode::GSM_ERROR;
}

#endif // ARDUINO
