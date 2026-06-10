// Sistema de Alarmas Comunitarias / Botón de Pánico (COMSECA-Ambato)
// Fase 0: esqueleto arquitectónico (Config, capas en lib/, logger, máquina de estados).
// Fase 1: botón de pánico por ISR -> cola -> AlarmController; LED de estado; GPIO de disparo.
// Fase 2: audio por tipo de evento (DFPlayer Mini) inyectado en el controlador.
// Fase 3: log de eventos en MicroSD (VSPI) con mutex.
//
// main.cpp es SÓLO composición: crea instancias, las inyecta y lanza las tareas.
#include <Arduino.h>

#include "Config.h"
#include "Events.h"
#include "Logger.h"
#include "GpioAlarmOutput.h"
#include "Mp3Player.h"
#include "SdStorage.h"
#include "AlarmController.h"

// --- Composición e inyección de dependencias ---
static GpioAlarmOutput alarmOutput(Config::PIN_RELAY);                                  // salida concreta (HAL)
static Mp3Player       audioPlayer(Serial1, Config::PIN_MP3_RX, Config::PIN_MP3_TX,     // DFPlayer en UART1 (9600)
                                   Config::MP3_BAUD);
static SdStorage       storage(Config::PIN_SD_CS, Config::PIN_SD_SCK,                   // MicroSD por VSPI
                               Config::PIN_SD_MISO, Config::PIN_SD_MOSI,
                               Config::LOG_EVENTS_PATH);
static AlarmController controller(alarmOutput, audioPlayer, storage);                   // recibe interfaces

static QueueHandle_t eventQueue = nullptr;   // bus de eventos del sistema

// --- ISR del botón de pánico (Fase 1) ---
// Mínima (criterio "Siempre"): sólo encola el evento, sin Serial/delay/malloc.
// El antirrebote vive en TaskController, no aquí.
void IRAM_ATTR buttonISR() {
  const EventType ev = EventType::BTN_PANIC;
  BaseType_t hpw = pdFALSE;
  xQueueSendFromISR(eventQueue, &ev, &hpw);
  if (hpw) portYIELD_FROM_ISR();
}

// --- TaskController: único consumidor de la cola y único escritor del log Serial ---
static void TaskController(void*) {
  EventType ev;
  uint32_t lastBtnMs = 0;
  for (;;) {
    if (xQueueReceive(eventQueue, &ev, portMAX_DELAY) == pdTRUE) {
      // Antirrebote por software del pulsador (marca de tiempo en la tarea).
      if (ev == EventType::BTN_PANIC) {
        const uint32_t now = millis();
        if (now - lastBtnMs < Config::BTN_DEBOUNCE_MS) continue;
        lastBtnMs = now;
      }
      controller.onEvent(ev);
    }
  }
}

// --- TaskStatusLed: refleja el estado en el LED (latido en ARMED/SILENCED, fijo en TRIGGERED) ---
// Único escritor de PIN_LED; lee el estado atómico del controlador.
static void TaskStatusLed(void*) {
  pinMode(Config::PIN_LED, OUTPUT);
  bool on = false;
  for (;;) {
    if (controller.state() == AlarmState::TRIGGERED) {
      digitalWrite(Config::PIN_LED, HIGH);              // encendido fijo
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      on = !on;                                         // latido
      digitalWrite(Config::PIN_LED, on ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(Config::LED_BLINK_ARMED_MS));
    }
  }
}

// --- TaskSerialConsole: inyección manual de eventos para la demo (sin hardware extra) ---
// p = pánico (BTN),  m = simula SMS,  r = simula REST,  s = silenciar,  a = ack.
// 'm' y 'r' permiten oír pistas DISTINTAS y demostrar "audio por tipo" / ">=2 audios"
// antes de tener las fuentes reales (SMS en Fase 4, REST en Fase 6). Único LECTOR del Serial.
static void TaskSerialConsole(void*) {
  for (;;) {
    if (Serial.available()) {
      const int c = Serial.read();
      EventType ev;
      bool valid = true;
      switch (c) {
        case 'p': case 'P': ev = EventType::BTN_PANIC; break;
        case 'm': case 'M': ev = EventType::SMS_IN;    break;
        case 'r': case 'R': ev = EventType::REST_IN;   break;
        case 's': case 'S': ev = EventType::SILENCE;   break;
        case 'a': case 'A': ev = EventType::ACK;       break;
        default:            valid = false;             break;
      }
      if (valid) xQueueSend(eventQueue, &ev, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(Config::LOG_BAUD);
  delay(50);
  Logger::info("=== Alarmas Comunitarias FW — Fase 0/1/2/3 ===");

  alarmOutput.begin();
  if (audioPlayer.initialize() != ErrorCode::OK) {
    Logger::warn("Audio no disponible; el sistema sigue (boton/LED/log/REST igual operan)");
  }
  if (storage.initialize() != ErrorCode::OK) {
    Logger::warn("MicroSD no disponible; el sistema sigue sin registrar eventos");
  } else {
    storage.printLog();   // muestra el log persistido (demuestra continuidad tras reinicio)
  }

  eventQueue = xQueueCreate(Config::CTRL_QUEUE_LEN, sizeof(EventType));
  if (eventQueue == nullptr) {
    Logger::error("No se pudo crear eventQueue; sistema detenido");
    return;
  }

  // Pulsador NA a GND con pull-up interno -> flanco de bajada al presionar.
  pinMode(Config::PIN_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Config::PIN_BTN), buttonISR, FALLING);

  Logger::info("Sistema ARMADO. Teclas: p=panico m=SMS r=REST s=silenciar.");

  // Tareas al final: hasta aquí el único escritor del Serial fue setup().
  // WiFi/BT irán en el core 0; la lógica en el core 1 (recomendación de la skill).
  xTaskCreatePinnedToCore(TaskController,    "ctrl",    4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(TaskStatusLed,     "led",     2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskSerialConsole, "console", 2048, nullptr, 1, nullptr, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));   // el trabajo vive en las tareas
}
