// Host unit tests for the contract-shaped telemetry JSON.
#include <unity.h>

#include <cstring>

#include "telemetry_frame.hpp"

void test_builds_valid_contract_shape_and_clamps_values() {
  const float samples[4] = {0.1f, 0.2f, 0.3f, 0.4f};
  syncfit::TelemetryData data;
  data.device_id = "syncfit-esp32-test";
  data.session_id = "A1B2C3D4";
  data.timestamp = "2026-10-27T21:30:00Z";
  data.modality = "GESTATIONAL";
  data.day_or_week = 99;
  data.delta_temperature_c = 5.0f;
  data.rmssd_hrv_ms = -4.0f;
  data.isometric_force_loss_pct = 150.0f;
  data.heart_rate_bpm = 250.0f;
  data.force_kg = -3.0f;
  data.object_temperature_c = 80.0f;
  data.finger_present = true;
  data.ppg_samples = samples;
  data.ppg_count = 4;

  char buffer[1024] = {};
  TEST_ASSERT_TRUE(syncfit::buildTelemetryJson(data, buffer, sizeof(buffer)));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"schema_version\":\"1.0.0\""));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"day_or_week\":42"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"delta_temperature_c\":2.000"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"rmssd_hrv_ms\":0.00"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"isometric_force_loss_pct\":100.00"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"heart_rate_bpm\":240.0"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"force_kg\":0.00"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"object_temperature_c\":60.00"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"finger_present\":true"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"window_size\":4"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"samples\":[0.100,0.200,0.300,0.400]"));
}

void test_rejects_empty_or_oversized_windows() {
  char buffer[1024] = {};
  syncfit::TelemetryData data;
  data.device_id = "d";
  data.session_id = "s";
  data.timestamp = "t";
  data.modality = "MENSTRUAL_CYCLE";
  data.ppg_count = 0;
  TEST_ASSERT_FALSE(syncfit::buildTelemetryJson(data, buffer, sizeof(buffer)));

  data.ppg_count = 257;
  const float samples[1] = {0.0f};
  data.ppg_samples = samples;
  TEST_ASSERT_FALSE(syncfit::buildTelemetryJson(data, buffer, sizeof(buffer)));
}

void test_rejects_small_buffer() {
  const float samples[1] = {0.0f};
  syncfit::TelemetryData data;
  data.device_id = "d";
  data.session_id = "s";
  data.timestamp = "t";
  data.modality = "MENSTRUAL_CYCLE";
  data.ppg_samples = samples;
  data.ppg_count = 1;
  char small[16] = {};
  TEST_ASSERT_FALSE(syncfit::buildTelemetryJson(data, small, sizeof(small)));
}

void test_builds_fast_force_message() {
  char buffer[96] = {};
  TEST_ASSERT_TRUE(syncfit::buildForceJson(8.437f, 12.34f, buffer, sizeof(buffer)));
  TEST_ASSERT_EQUAL_STRING("{\"type\":\"force\",\"force_kg\":8.44,\"loss_pct\":12.3}", buffer);
  TEST_ASSERT_TRUE(syncfit::buildForceJson(-2.0f, 150.0f, buffer, sizeof(buffer)));
  TEST_ASSERT_EQUAL_STRING("{\"type\":\"force\",\"force_kg\":0.00,\"loss_pct\":100.0}", buffer);
  char tiny[8] = {};
  TEST_ASSERT_FALSE(syncfit::buildForceJson(1.0f, 1.0f, tiny, sizeof(tiny)));
}

void test_force_gate_sends_changes_and_keepalive_only() {
  syncfit::ForceSendGate gate;
  TEST_ASSERT_TRUE(gate.shouldSend(0.10f, 0.0f, 0));      // first value
  TEST_ASSERT_FALSE(gate.shouldSend(0.11f, 0.0f, 150));   // tiny change, no keep-alive yet
  TEST_ASSERT_FALSE(gate.shouldSend(3.00f, 0.0f, 50));    // big change but < 100 ms
  TEST_ASSERT_TRUE(gate.shouldSend(3.00f, 0.0f, 200));    // big change
  TEST_ASSERT_FALSE(gate.shouldSend(3.02f, 0.2f, 900));   // idle
  TEST_ASSERT_TRUE(gate.shouldSend(3.02f, 0.2f, 1300));   // keep-alive after 1 s
  TEST_ASSERT_TRUE(gate.shouldSend(3.02f, 1.0f, 1450));   // loss moved >= 0.5 %
}

#include "http_date.hpp"

void test_parses_http_date_header() {
  std::int64_t epoch = 0;
  TEST_ASSERT_TRUE(syncfit::parseHttpDate("Wed, 19 Nov 2026 14:03:21 GMT", epoch));
  TEST_ASSERT_EQUAL_INT64(1795097001LL, epoch);
  TEST_ASSERT_TRUE(syncfit::parseHttpDate("Thu, 01 Jan 2026 00:00:00 GMT", epoch));
  TEST_ASSERT_EQUAL_INT64(1767225600LL, epoch);
  TEST_ASSERT_FALSE(syncfit::parseHttpDate("garbage", epoch));
  TEST_ASSERT_FALSE(syncfit::parseHttpDate("Wed, 19 Foo 2026 14:03:21 GMT", epoch));
  TEST_ASSERT_FALSE(syncfit::parseHttpDate(nullptr, epoch));
}
