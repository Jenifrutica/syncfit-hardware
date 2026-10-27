// Shared state between the sensor, force and telemetry tasks.
// Access is protected by a FreeRTOS mutex in main.cpp.

#pragma once

#include <cstddef>

#include "config.hpp"

namespace syncfit {

struct AppState {
  float ppg[config::kPpgWindowSize] = {};
  std::size_t ppg_count = 0;
  float rmssd_ms = 0.0f;
  float heart_rate_bpm = 0.0f;
  float delta_temperature_c = 0.0f;
  float object_temperature_c = 0.0f;
  float force_kg = 0.0f;
  float force_loss_pct = 0.0f;
  bool finger_present = false;
  bool temperature_valid = false;
  bool force_valid = false;
};

}  // namespace syncfit
