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
  float heart_rate_bpm = 0.0f;
  float force_kg = 0.0f;
  float object_temperature_c = 0.0f;
  bool finger_present = false;
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

inline float clampHeartRate(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 240.0f) return 240.0f;
  return value;
}

inline float clampForceKg(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 500.0f) return 500.0f;
  return value;
}

inline float clampObjectTemperature(float value) {
  if (value < -20.0f) return -20.0f;
  if (value > 60.0f) return 60.0f;
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
          "\"isometric_force_loss_pct\":%.2f,\"heart_rate_bpm\":%.1f,"
          "\"force_kg\":%.2f,\"object_temperature_c\":%.2f,"
          "\"finger_present\":%s},",
          detail::clampDeltaTemperature(data.delta_temperature_c),
          detail::clampRmssd(data.rmssd_hrv_ms),
          detail::clampForceLoss(data.isometric_force_loss_pct),
          detail::clampHeartRate(data.heart_rate_bpm),
          detail::clampForceKg(data.force_kg),
          detail::clampObjectTemperature(data.object_temperature_c),
          data.finger_present ? "true" : "false")) {
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

// Fast force message sent every ~100 ms next to the 2.56 s frame, so the app's
// force step reacts without waiting for the full telemetry window.
inline bool buildForceJson(float force_kg, float loss_pct, char* out, std::size_t capacity) {
  if (out == nullptr || capacity == 0) return false;
  const int written = std::snprintf(out, capacity,
                                    "{\"type\":\"force\",\"force_kg\":%.2f,\"loss_pct\":%.1f}",
                                    detail::clampForceKg(force_kg),
                                    detail::clampForceLoss(loss_pct));
  return written > 0 && static_cast<std::size_t>(written) < capacity;
}

// Throttle for the fast force channel: send at most every `min_ms`, and only
// when the force or the loss changed enough, plus a keep-alive every
// `keepalive_ms`. An idle platform then costs ~1 msg/s instead of 10 msg/s,
// which keeps the TLS socket healthy on slow networks.
struct ForceSendGate {
  float last_kg = -1000.0f;
  float last_loss = -1000.0f;
  std::uint32_t last_ms = 0;
  bool sent_once = false;

  bool shouldSend(float kg, float loss, std::uint32_t now_ms, std::uint32_t min_ms = 100,
                  std::uint32_t keepalive_ms = 1000, float kg_step = 0.05f, float loss_step = 0.5f) {
    const std::uint32_t elapsed = now_ms - last_ms;
    if (sent_once && elapsed < min_ms) return false;
    const float dkg = kg > last_kg ? kg - last_kg : last_kg - kg;
    const float dloss = loss > last_loss ? loss - last_loss : last_loss - loss;
    const bool changed = !sent_once || dkg >= kg_step || dloss >= loss_step;
    if (!changed && elapsed < keepalive_ms) return false;
    last_kg = kg;
    last_loss = loss;
    last_ms = now_ms;
    sent_once = true;
    return true;
  }
};

}  // namespace syncfit
