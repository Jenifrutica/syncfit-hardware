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
  data.ppg_samples = samples;
  data.ppg_count = 4;

  char buffer[1024] = {};
  TEST_ASSERT_TRUE(syncfit::buildTelemetryJson(data, buffer, sizeof(buffer)));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"schema_version\":\"1.0.0\""));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"day_or_week\":42"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"delta_temperature_c\":2.000"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"rmssd_hrv_ms\":0.00"));
  TEST_ASSERT_NOT_NULL(std::strstr(buffer, "\"isometric_force_loss_pct\":100.00"));
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
