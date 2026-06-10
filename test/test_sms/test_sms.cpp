// Tests de la lógica pura de SMS (SmsCommand) en env:native — sin ESP32/SIM800L.
#include <unity.h>
#include "SmsCommand.h"
#include "Events.h"

void setUp(void)    {}
void tearDown(void) {}

// --- parseCmtHeader: extraer el remitente del +CMT ---
void test_parse_cmt_extrae_remitente(void) {
  std::string sender;
  bool ok = SmsCommand::parseCmtHeader(
      "+CMT: \"+593991234567\",\"\",\"25/06/09,22:00:00+00\"", sender);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("+593991234567", sender.c_str());
}

void test_parse_cmt_sin_comillas_falla(void) {
  std::string sender;
  TEST_ASSERT_FALSE(SmsCommand::parseCmtHeader("+CMT: basura", sender));
}

// --- toEvent: cuerpo del SMS -> evento ---
void test_panico_dispara(void) {
  EventType ev;
  TEST_ASSERT_TRUE(SmsCommand::toEvent("PANICO", ev));
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::SMS_IN), static_cast<int>(ev));
}

void test_minusculas_y_espacios_disparan(void) {
  EventType ev;
  TEST_ASSERT_TRUE(SmsCommand::toEvent("  emergencia ahora ", ev));
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::SMS_IN), static_cast<int>(ev));
}

void test_silenciar_mapea_a_silence(void) {
  EventType ev;
  TEST_ASSERT_TRUE(SmsCommand::toEvent("Silenciar", ev));
  TEST_ASSERT_EQUAL(static_cast<int>(EventType::SILENCE), static_cast<int>(ev));
}

void test_texto_desconocido_no_dispara(void) {
  EventType ev;
  TEST_ASSERT_FALSE(SmsCommand::toEvent("hola que tal", ev));
}

// --- isAllowedSender: allow-list ---
void test_remitente_autorizado(void) {
  TEST_ASSERT_TRUE(SmsCommand::isAllowedSender(
      "+593991234567", "+593991234567,+593988888888"));
}

void test_remitente_autorizado_con_espacios_en_lista(void) {
  TEST_ASSERT_TRUE(SmsCommand::isAllowedSender(
      "+593988888888", " +593991234567 , +593988888888 "));
}

void test_remitente_no_autorizado(void) {
  TEST_ASSERT_FALSE(SmsCommand::isAllowedSender(
      "+593900000000", "+593991234567,+593988888888"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_cmt_extrae_remitente);
  RUN_TEST(test_parse_cmt_sin_comillas_falla);
  RUN_TEST(test_panico_dispara);
  RUN_TEST(test_minusculas_y_espacios_disparan);
  RUN_TEST(test_silenciar_mapea_a_silence);
  RUN_TEST(test_texto_desconocido_no_dispara);
  RUN_TEST(test_remitente_autorizado);
  RUN_TEST(test_remitente_autorizado_con_espacios_en_lista);
  RUN_TEST(test_remitente_no_autorizado);
  return UNITY_END();
}
