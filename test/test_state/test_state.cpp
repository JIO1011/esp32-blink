// Tests de la máquina de estados (AlarmController) en env:native — sin ESP32.
// El hardware se sustituye por un fake de IAlarmOutput.
#include <unity.h>
#include "AlarmController.h"
#include "IAlarmOutput.h"

// Fake de la salida de potencia: cuenta activaciones sin tocar hardware.
class FakeAlarmOutput : public IAlarmOutput {
public:
  int  activations   = 0;
  int  deactivations = 0;
  bool active        = false;
  void activate()   override { ++activations;   active = true;  }
  void deactivate() override { ++deactivations; active = false; }
};

void setUp(void)    {}
void tearDown(void) {}

void test_estado_inicial_es_armed(void) {
  FakeAlarmOutput out;
  AlarmController c(out);
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::ARMED), static_cast<int>(c.state()));
  TEST_ASSERT_FALSE(out.active);
}

void test_boton_dispara_y_activa_salida(void) {
  FakeAlarmOutput out;
  AlarmController c(out);
  c.onEvent(EventType::BTN_PANIC);
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(c.state()));
  TEST_ASSERT_TRUE(out.active);
  TEST_ASSERT_EQUAL(1, out.activations);
}

void test_disparo_es_idempotente(void) {
  FakeAlarmOutput out;
  AlarmController c(out);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::BTN_PANIC);   // ya activa
  c.onEvent(EventType::SMS_IN);      // otra fuente, sigue activa
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(c.state()));
  TEST_ASSERT_EQUAL(1, out.activations);   // no se reactiva
}

void test_silence_silencia_y_desactiva(void) {
  FakeAlarmOutput out;
  AlarmController c(out);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::SILENCE);
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::SILENCED), static_cast<int>(c.state()));
  TEST_ASSERT_FALSE(out.active);
  TEST_ASSERT_EQUAL(1, out.deactivations);
}

void test_redispara_desde_silenced(void) {
  FakeAlarmOutput out;
  AlarmController c(out);
  c.onEvent(EventType::BTN_PANIC);
  c.onEvent(EventType::SILENCE);
  c.onEvent(EventType::REST_IN);     // REST también dispara
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::TRIGGERED), static_cast<int>(c.state()));
  TEST_ASSERT_TRUE(out.active);
}

void test_silence_en_armed_no_hace_nada(void) {
  FakeAlarmOutput out;
  AlarmController c(out);
  c.onEvent(EventType::SILENCE);     // silenciar sin estar disparada
  TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::ARMED), static_cast<int>(c.state()));
  TEST_ASSERT_EQUAL(0, out.activations);
  TEST_ASSERT_EQUAL(0, out.deactivations);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_estado_inicial_es_armed);
  RUN_TEST(test_boton_dispara_y_activa_salida);
  RUN_TEST(test_disparo_es_idempotente);
  RUN_TEST(test_silence_silencia_y_desactiva);
  RUN_TEST(test_redispara_desde_silenced);
  RUN_TEST(test_silence_en_armed_no_hace_nada);
  return UNITY_END();
}
