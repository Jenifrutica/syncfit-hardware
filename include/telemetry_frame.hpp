// Contract-shaped telemetry JSON without dynamic allocation.
// Matches syncfit-contracts TelemetryFrame 1.0.0:
// device_id, session_id, timestamp, modality, day_or_week, biomarkers, ppg_window.

#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace syncfit {

struct TelemetryData {
  const char* device_id = "";
  const char* session_id = "";
  const char* timestamp = "";
  const char* modality = "MENSTRUAL_CYCLE";
  int day_or_week = 14;
  float delta_temperature_c = 0.0f;
  float rmssd_hrv_ms = 0.0f;
  float isometric_force_loss_pct = 0.0f;
  const float* ppg_samples = nullptr;
  std::size_t ppg_count = 0;
};

namespace detail {

inline bool append(char* out, std::size_t capacity, std::size_t& used, const char* text) {
  if (out == nullptr || capacity == 0) return false;
  const std::size_t length = std::strlen(text);
  if (used + length + 1 > capacity) return false;
  std::memcpy(out + used, text, length);
  used += length;
  out[used] = '\0';
  return true;
}

inline bool appendf(char* out, std::size_t capacity, std::size_t& used, const char* format, ...) {
  if (out == nullptr || capacity == 0 || used >= capacity) return false;
  va_list args;
  va_start(args, format);
  const int written = std::vsnprintf(out + used, capacity - used, format, args);
  va_end(args);
  if (written < 0) return false;
  if (static_cast<std::size_t>(written) >= capacity - used) {
    out[capacity - 1] = '\0';
    return false;
  }
  used += static_cast<std::size_t>(written);
  return true;
}

inline float clampDeltaTemperature(float value) {
  if (value < -2.0f) return -2.0f;
  if (value > 2.0f) return 2.0f;
  return value;
}

inline float clampRmssd(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 300.0f) return 300.0f;
  return value;
}

inline float clampForceLoss(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 100.0f) return 100.0f;
  return value;
}

}  // namespace detail

inline bool buildTelemetryJson(const TelemetryData& data, char* out, std::size_t capacity) {
  if (out == nullptr || capacity == 0) return false;
  if (data.ppg_samples == nullptr || data.ppg_count == 0) return false;
  if (data.ppg_count > 256) return false;
  if (data.device_id == nullptr || data.session_id == nullptr ||
      data.timestamp == nullptr || data.modality == nullptr) {
    return false;
  }

  out[0] = '\0';
  std::size_t used = 0;
  const int day = data.day_or_week < 1 ? 1 : (data.day_or_week > 42 ? 42 : data.day_or_week);

  if (!detail::append(out, capacity, used,
                      "{\"schema_version\":\"1.0.0\",\"device_id\":\"")) return false;
  if (!detail::append(out, capacity, used, data.device_id)) return false;
  if (!detail::append(out, capacity, used, "\",\"session_id\":\"")) return false;
  if (!detail::append(out, capacity, used, data.session_id)) return false;
  if (!detail::append(out, capacity, used, "\",\"timestamp\":\"")) return false;
  if (!detail::append(out, capacity, used, data.timestamp)) return false;
  if (!detail::append(out, capacity, used, "\",\"modality\":\"")) return false;
  if (!detail::append(out, capacity, used, data.modality)) return false;
  if (!detail::appendf(out, capacity, used, "\",\"day_or_week\":%d,", day)) return false;

  if (!detail::appendf(
          out, capacity, used,
          "\"biomarkers\":{\"delta_temperature_c\":%.3f,\"rmssd_hrv_ms\":%.2f,"
          "\"isometric_force_loss_pct\":%.2f},",
          detail::clampDeltaTemperature(data.delta_temperature_c),
          detail::clampRmssd(data.rmssd_hrv_ms),
          detail::clampForceLoss(data.isometric_force_loss_pct))) {
    return false;
  }

  if (!detail::appendf(
          out, capacity, used,
          "\"ppg_window\":{\"sample_rate_hz\":100,\"window_size\":%u,\"samples\":[",
          static_cast<unsigned>(data.ppg_count))) {
    return false;
  }

  for (std::size_t i = 0; i < data.ppg_count; ++i) {
    if (!detail::appendf(out, capacity, used, i == 0 ? "%.3f" : ",%.3f",
                         static_cast<double>(data.ppg_samples[i]))) {
      return false;
    }
  }

  if (!detail::append(out, capacity, used, "]}}")) return false;
  return true;
}

}  // namespace syncfit
