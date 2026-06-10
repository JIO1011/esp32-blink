#pragma once
#include <cstdint>

// Fuente ÚNICA de pines, baudrates y tiempos (criterio "Siempre" de la skill de
// firmware: nada de números mágicos sueltos). Pinout según la skill de hardware
// alarmas-comunitarias-hw para ESP32 DevKit V1 (WROOM-32).
namespace Config {

  // --- Pines de periféricos (asignados, se usan en su fase) ---
  constexpr uint8_t PIN_SIM_RX   = 26;  // SIM800L TX  -> ESP32 RX            (Fase 4)
  constexpr uint8_t PIN_SIM_TX   = 27;  // ESP32 TX    -> SIM800L RX (divisor)(Fase 4)
  constexpr uint8_t PIN_SIM_RST  = 33;  // Reset SIM800L (opcional)           (Fase 4)
  constexpr uint8_t PIN_MP3_RX   = 16;  // DFPlayer TX -> ESP32 RX            (Fase 2)
  constexpr uint8_t PIN_MP3_TX   = 17;  // ESP32 TX    -> DFPlayer RX (1 kΩ)  (Fase 2)
  constexpr uint8_t PIN_SD_SCK   = 18;  // MicroSD VSPI SCK                    (Fase 3)
  constexpr uint8_t PIN_SD_MISO  = 19;  // MicroSD VSPI MISO                   (Fase 3)
  constexpr uint8_t PIN_SD_MOSI  = 23;  // MicroSD VSPI MOSI                   (Fase 3)
  constexpr uint8_t PIN_SD_CS    = 5;   // MicroSD VSPI CS                     (Fase 3)
  constexpr uint8_t PIN_W5500_CS = 4;   // LAN W5500 CS (VSPI compartido)     (Fase 8)

  // --- E/S de la Fase 1 ---
  constexpr uint8_t PIN_BTN   = 25;  // Pulsador de pánico (NA a GND, INPUT_PULLUP). Libre, con pull-up, sin rol de boot.
  constexpr uint8_t PIN_RELAY = 13;  // GPIO de disparo: placeholder de la etapa de potencia de producción.
  constexpr uint8_t PIN_LED   = 2;   // LED de estado en placa (strapping; válido como indicador post-arranque).

  // --- Baudrates ---
  constexpr uint32_t LOG_BAUD = 115200;
  constexpr uint32_t GSM_BAUD = 9600;
  constexpr uint32_t MP3_BAUD = 9600;

  // --- Tiempos / capacidades ---
  constexpr uint32_t BTN_DEBOUNCE_MS    = 250;  // antirrebote por software (en la tarea, no en la ISR)
  constexpr uint32_t LED_BLINK_ARMED_MS = 800;  // periodo de latido en ARMED/SILENCED
  constexpr uint32_t CTRL_QUEUE_LEN     = 10;   // capacidad de la cola de eventos

  // --- Mapa de audios DFPlayer (/mp3/000x.mp3) (se usa en Fase 2) ---
  constexpr uint16_t TRACK_PANIC      = 1;
  constexpr uint16_t TRACK_SUSPICIOUS = 2;
  constexpr uint16_t TRACK_DETERRENT  = 3;

  // --- Almacenamiento (Fase 3) ---
  constexpr char LOG_EVENTS_PATH[] = "/eventos.csv";   // log en la MicroSD del ESP32 (VSPI)

  // --- SIM800L / SMS (Fase 4) ---
  // Remitentes autorizados para activar por SMS (formato internacional, separados por comas).
  // Demo: aquí. Producción/Fase 5: configurar por SoftAP/NVS y/o mover a secrets.h fuera de git.
  constexpr char SMS_ALLOWLIST[] = "+593991234567,+593988888888";

} // namespace Config
