// Tests de la máquina de estados (AlarmController) en env:native — sin ESP32.
// El hardware se sustituye por fakes de IAlarmOutput, IAudioPlayer e IStorage.
#include <unity.h>
#include "Config.h"
#include "AlarmController.h"
#include "IAlarmOutput.h"
#include "IAudioPlayer.h"
#include "IStorage.h"

// Fake de la salida de potencia: cuenta activaciones sin tocar hardware.
class FakeAlarmOutput : public IAlarmOutput {
public:
  int  activations   = 0;
  int  deactivations = 0;
  bool active        = false;
  void activate()   override { ++activations;   active = true;  }
  void deactivate() override { ++deactivations; active = false; }
};

// Fake del reproductor: registra la última pista y los conteos.
class FakeAudioPlayer : public IAudioPlayer {
public:
  int      playCount = 0;
  int      stopCount = 0;
  uint16_t lastTrack = 0;
  ErrorCode initialize() override { return ErrorCode::OK; }
  void play(uint16_t track) override { ++playCount; lastTrack = track; }
  void stop() override { ++stopCount; }
};

// Fake del almacenamiento: registra el último evento logueado y el conteo.
class FakeStorage : public IStorage {
public:
  int        logCount   = 0;
  EventType  lastOrigin = EventType::TICK;
  AlarmState lastState  = AlarmState::IDLE;
  ErrorCode initialize() override { return ErrorCode::OK; }
  ErrorCode logEvent(EventType origin, AlarmState state) override {
    ++logCount; lastOrigin = origin; lastState = state; return ErrorCode::OK;
  }
};

void setUp(void)    {}
void tearDown(void) {}

void test_estado_inicial_es_armed(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::ARMED), static_cast<int>(c.state()));
  TEST_ASSERT_FALSE(out.active);
  TEST_ASSERT_EQUAL(0, audio.playCount);
  TEST_ASSERT_EQUAL(0, store.logCount);
}

void test_boton_dispara_activa_salida_y_pista_panico(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::BTN_PANIC);
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(c.state()));
  TEST_ASSERT_TRUE(out.active);
  TEST_ASSERT_EQUAL(1, out.activations);
  TEST_ASSERT_EQUAL(1, audio.playCount);
  TEST_ASSERT_EQUAL_UINT16(Config::TRACK_PANIC, audio.lastTrack);
}

void test_sms_reproduce_pista_sospechoso(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::SMS_IN);
  TEST_ASSERT_EQUAL_UINT16(Config::TRACK_SUSPICIOUS, audio.lastTrack);
}

void test_rest_reproduce_pista_disuasivo(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::REST_IN);
  TEST_ASSERT_EQUAL_UINT16(Config::TRACK_DETERRENT, audio.lastTrack);
}

void test_disparo_registra_evento_en_storage(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::BTN_PANIC);
  TEST_ASSERT_EQUAL(1, store.logCount);
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::BTN_PANIC), static_cast<int>(store.lastOrigin));
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(store.lastState));
}

void test_silence_registra_evento_en_storage(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::SILENCE);
  TEST_ASSERT_EQUAL(2, store.logCount);   // disparo + silencio
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::SILENCE), static_cast<int>(store.lastOrigin));
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::SILENCED), static_cast<int>(store.lastState));
}

void test_disparo_es_idempotente(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::BTN_PANIC);   // ya activa
  c.onEvent(EventType::SMS_IN);      // otra fuente, sigue activa
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(c.state()));
  TEST_ASSERT_EQUAL(1, out.activations);   // no se reactiva
  TEST_ASSERT_EQUAL(1, audio.playCount);   // no se re-reproduce
  TEST_ASSERT_EQUAL(1, store.logCount);    // sólo se registra el primer disparo
}

void test_silence_silencia_desactiva_y_para_audio(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::SILENCE);
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::SILENCED), static_cast<int>(c.state()));
  TEST_ASSERT_FALSE(out.active);
  TEST_ASSERT_EQUAL(1, out.deactivations);
  TEST_ASSERT_EQUAL(1, audio.stopCount);
}

void test_redispara_desde_silenced(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::SILENCE);
  c.onEvent(EventType::REST_IN);     // REST también dispara
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(c.state()));
  TEST_ASSERT_TRUE(out.active);
}

void test_silence_en_armed_no_hace_nada(void) {
  FakeAlarmOutput out; FakeAudioPlayer audio; FakeStorage store;
  AlarmController c(out, audio, store);
  c.onEvent(EventType::SILENCE);     // silenciar sin estar disparada
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::ARMED), static_cast<int>(c.state()));
  TEST_ASSERT_EQUAL(0, out.activations);
  TEST_ASSERT_EQUAL(0, audio.playCount);
  TEST_ASSERT_EQUAL(0, store.logCount);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_estado_inicial_es_armed);
  RUN_TEST(test_boton_dispara_activa_salida_y_pista_panico);
  RUN_TEST(test_sms_reproduce_pista_sospechoso);
  RUN_TEST(test_rest_reproduce_pista_disuasivo);
  RUN_TEST(test_disparo_registra_evento_en_storage);
  RUN_TEST(test_silence_registra_evento_en_storage);
  RUN_TEST(test_disparo_es_idempotente);
  RUN_TEST(test_silence_silencia_desactiva_y_para_audio);
  RUN_TEST(test_redispara_desde_silenced);
  RUN_TEST(test_silence_en_armed_no_hace_nada);
  return UNITY_END();
}
