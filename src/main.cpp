// Sistema de Alarmas Comunitarias / Botón de Pánico (COMSECA-Ambato)
// Fase 0: esqueleto arquitectónico (Config, capas en lib/, logger, máquina de estados).
// Fase 1: botón de pánico por ISR -> cola -> AlarmController; LED de estado; GPIO de disparo.
// Fase 2: audio por tipo de evento (DFPlayer Mini) inyectado en el controlador.
// Fase 3: log de eventos en MicroSD (VSPI) con mutex.
// Fase 4: activación por SMS (SIM800L) -> TaskGsm publica eventos en la misma cola.
//
// main.cpp es SÓLO composición: crea instancias, las inyecta y lanza las tareas.
#include <Arduino.h>
#include <string>
#include <HTTPClient.h>

#include "Config.h"
#include "Events.h"
#include "Logger.h"
#include "GpioAlarmOutput.h"
#include "Mp3Player.h"
#include "SdStorage.h"
#include "Sim800Driver.h"
#include "SmsCommand.h"
#include "ConfigStore.h"
#include "NetPortal.h"
#include "RestApi.h"
#include "AlarmController.h"

// --- Composición e inyección de dependencias ---
static GpioAlarmOutput alarmOutput(Config::PIN_RELAY);                                  // salida concreta (HAL)
static Mp3Player       audioPlayer(Serial1, Config::PIN_MP3_RX, Config::PIN_MP3_TX,     // DFPlayer en UART1 (9600)
                                   Config::MP3_BAUD, Config::MP3_VOLUME);
static SdStorage       storage(Config::PIN_SD_CS, Config::PIN_SD_SCK,                   // MicroSD por VSPI
                               Config::PIN_SD_MISO, Config::PIN_SD_MOSI,
                               Config::LOG_EVENTS_PATH);
static Sim800Driver    gsm(Serial2, Config::PIN_SIM_RX, Config::PIN_SIM_TX,             // SIM800L en UART2 (9600)
                           Config::GSM_BAUD, Config::GSM_APN);
static AlarmController controller(alarmOutput, audioPlayer, storage);                   // recibe interfaces
static ConfigStore     configStore;                                                     // config persistente (NVS)
static WebServer       webServer(80);                                                   // HTTP compartido (portal + REST)
static NetPortal       netPortal(configStore, Config::AP_SSID, Config::AP_PASS, webServer);

static QueueHandle_t eventQueue = nullptr;   // bus de eventos del sistema

// Notificación saliente (Fase 7): TaskController la encola en cada transición real;
// TaskGsm la envía por la red disponible (WiFi o GPRS) sin bloquear al controlador.
struct Notification { EventType origin; AlarmState state; };
static QueueHandle_t notifyQueue = nullptr;

// API REST: cada operación entrante se publica como evento en la MISMA cola del sistema.
static RestApi         restApi(webServer, controller, storage,
                               [](EventType ev) { if (eventQueue) xQueueSend(eventQueue, &ev, 0); });

// --- ISR del botón de pánico (Fase 1) ---
// Mínima (criterio "Siempre"): sólo encola el evento, sin Serial/delay/malloc.
void IRAM_ATTR buttonISR() {
  const EventType ev = EventType::BTN_PANIC;
  BaseType_t hpw = pdFALSE;
  xQueueSendFromISR(eventQueue, &ev, &hpw);
  if (hpw) portYIELD_FROM_ISR();
}

// --- TaskController: consume la cola y aplica la máquina de estados ---
static void TaskController(void*) {
  EventType ev;
  uint32_t lastBtnMs = 0;
  for (;;) {
    if (xQueueReceive(eventQueue, &ev, portMAX_DELAY) == pdTRUE) {
      if (ev == EventType::BTN_PANIC) {              // antirrebote por software (en la tarea)
        const uint32_t now = millis();
        if (now - lastBtnMs < Config::BTN_DEBOUNCE_MS) continue;
        lastBtnMs = now;
      }
      const AlarmState before = controller.state();
      controller.onEvent(ev);
      const AlarmState after = controller.state();
      if (after != before && notifyQueue) {          // hubo transición -> notificar al exterior
        Notification n{ev, after};
        xQueueSend(notifyQueue, &n, 0);              // no bloquea: el POST vive en TaskGsm
      }
    }
  }
}

// --- TaskStatusLed: latido en ARMED/SILENCED, fijo en TRIGGERED ---
static void TaskStatusLed(void*) {
  pinMode(Config::PIN_LED, OUTPUT);
  bool on = false;
  for (;;) {
    if (controller.state() == AlarmState::TRIGGERED) {
      digitalWrite(Config::PIN_LED, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      on = !on;
      digitalWrite(Config::PIN_LED, on ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(Config::LED_BLINK_ARMED_MS));
    }
  }
}

// --- TaskSerialConsole: inyección manual de eventos para la demo (sin hardware extra) ---
// p = pánico (BTN),  m = simula SMS,  r = simula REST,  s = silenciar,  a = ack.
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

// POST JSON por WiFi (HTTPClient). Para HTTPS haría falta WiFiClientSecure (ver nota Fase 7).
static bool postViaWifi(const char* url, const std::string& json) {
  HTTPClient http;
  if (!http.begin(url)) return false;
  http.addHeader("Content-Type", "application/json");
  const int code = http.POST(String(json.c_str()));
  http.end();
  return code >= 200 && code < 300;
}

// --- TaskGsm: única tarea que toca la UART2 (sin mutex). Hace dos cosas: ---
//   1) Envía las notificaciones salientes (Fase 7) por la red disponible (WiFi o GPRS).
//   2) Recibe SMS (Fase 4), valida remitente y publica el evento en la cola.
// Si no hay SIM800L, NO muere: sigue notificando por WiFi (el SMS queda deshabilitado).
static void TaskGsm(void*) {
  const bool gsmReady = (gsm.initialize() == ErrorCode::OK);
  if (!gsmReady)
    Logger::warn("SIM800L no disponible; SMS off (las notificaciones saldran por WiFi si hay)");

  std::string from, body;
  Notification n;
  for (;;) {
    // 1) Notificación saliente
    if (notifyQueue && xQueueReceive(notifyQueue, &n, 0) == pdTRUE) {
      const std::string json = std::string("{\"origen\":\"") + nameOf(n.origin) +
                               "\",\"estado\":\"" + nameOf(n.state) + "\"}";
      bool ok = false;
      const char* via = "ninguna";
      if (WiFi.status() == WL_CONNECTED) { via = "WiFi"; ok = postViaWifi(Config::WEBHOOK_URL, json); }
      else if (gsmReady)                 { via = "GPRS"; ok = (gsm.httpPostJson(Config::WEBHOOK_URL, json) == ErrorCode::OK); }
      Logger::info("Notificacion %s via %s: %s", ok ? "OK" : "FALLO", via, json.c_str());
    }
    // 2) SMS entrante (solo si hay módulo)
    if (gsmReady && gsm.pollIncoming(from, body)) {
      const std::string allow = configStore.smsAllowList(Config::SMS_ALLOWLIST).c_str();
      if (!SmsCommand::isAllowedSender(from, allow)) {
        Logger::warn("SMS de %s IGNORADO (no autorizado)", from.c_str());
      } else {
        EventType ev;
        if (SmsCommand::toEvent(body, ev)) {
          Logger::info("SMS de %s autorizado -> %s", from.c_str(), nameOf(ev));
          xQueueSend(eventQueue, &ev, 0);
        } else {
          Logger::warn("SMS de %s sin orden reconocida: '%s'", from.c_str(), body.c_str());
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// --- TaskNet: WiFi Station + SoftAP y portal de configuración (Fase 5) ---
// Va en el core 0 (donde corre el stack WiFi/BT del ESP32).
static void TaskNet(void*) {
  netPortal.begin();          // WiFi AP+STA + rutas del portal + server.begin()
  restApi.registerRoutes();   // monta /api/* en el MISMO WebServer
  for (;;) {
    netPortal.handle();       // webServer.handleClient()
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(Config::LOG_BAUD);
  Logger::begin();          // mutex de Serial ANTES de cualquier log concurrente
  delay(50);
  Logger::info("=== Alarmas Comunitarias FW — Fase 0/1/2/3/4/5/6/7 ===");

  configStore.begin();      // NVS: credenciales/parametros persistentes

  alarmOutput.begin();
  if (audioPlayer.initialize() != ErrorCode::OK) {
    Logger::warn("Audio no disponible; el sistema sigue (boton/LED/log/REST igual operan)");
  }
  if (storage.initialize() != ErrorCode::OK) {
    Logger::warn("MicroSD no disponible; el sistema sigue sin registrar eventos");
  } else {
    storage.printLog();     // muestra el log persistido (continuidad tras reinicio)
  }

  eventQueue  = xQueueCreate(Config::CTRL_QUEUE_LEN, sizeof(EventType));
  notifyQueue = xQueueCreate(8, sizeof(Notification));
  if (eventQueue == nullptr) {
    Logger::error("No se pudo crear eventQueue; sistema detenido");
    return;
  }

  // Pulsador NA a GND con pull-up interno -> flanco de bajada al presionar.
  pinMode(Config::PIN_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Config::PIN_BTN), buttonISR, FALLING);

  Logger::info("Sistema ARMADO. Teclas: p=panico m=SMS r=REST s=silenciar.");

  // El log Serial ya es thread-safe (Logger::begin), así que las tareas pueden loguear en paralelo.
  // WiFi/BT irán en el core 0; la lógica en el core 1 (recomendación de la skill).
  xTaskCreatePinnedToCore(TaskController,    "ctrl",    4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(TaskStatusLed,     "led",     2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskSerialConsole, "console", 2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskGsm,           "gsm",     4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskNet,           "net",     8192, nullptr, 1, nullptr, 0);  // WiFi/portal en core 0
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));   // el trabajo vive en las tareas
}
