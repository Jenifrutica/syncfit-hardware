// Automatic zero ("auto-tare") when the prototype starts or reconnects.
//
// The load cells drift a few hundred grams to several kilos between sessions
// (mechanical hysteresis, temperature). When armed, the gate waits until the
// platform is clearly at rest - the last `window` readings vary less than
// `max_span_kg` and stay under `max_abs_kg` - and then asks for a tare. It
// never zeroes during a push (unstable or heavy readings) and gives up after
// `timeout_ms`, keeping the previous zero.
//
// Host-testable: no Arduino headers.

#pragma once

#include <cstddef>
#include <cstdint>

namespace syncfit {

class AutoTareGate {
 public:
  static constexpr std::size_t kMaxWindow = 64;

  explicit AutoTareGate(std::size_t window = 30, float max_span_kg = 0.08f, float max_abs_kg = 6.0f,
                        std::uint32_t timeout_ms = 60000)
      : window_(window < kMaxWindow ? window : kMaxWindow),
        max_span_kg_(max_span_kg),
        max_abs_kg_(max_abs_kg),
        timeout_ms_(timeout_ms) {}

  void arm(std::uint32_t now_ms) {
    armed_ = true;
    armed_at_ = now_ms;
    count_ = 0;
    head_ = 0;
  }

  bool armed() const { return armed_; }

  // Feed one reading (kg, with the current zero). Returns true exactly once,
  // when the platform has been still long enough: the caller then tares.
  bool push(float kg, std::uint32_t now_ms) {
    if (!armed_) return false;
    if (now_ms - armed_at_ > timeout_ms_) {
      armed_ = false;  // give up: keep the previous zero
      return false;
    }
    values_[head_] = kg;
    head_ = (head_ + 1) % window_;
    if (count_ < window_) ++count_;
    if (count_ < window_) return false;
    float lo = values_[0], hi = values_[0];
    for (std::size_t i = 1; i < window_; ++i) {
      if (values_[i] < lo) lo = values_[i];
      if (values_[i] > hi) hi = values_[i];
    }
    const float mean = (lo + hi) * 0.5f;
    const float abs_mean = mean < 0 ? -mean : mean;
    if (hi - lo <= max_span_kg_ && abs_mean <= max_abs_kg_) {
      armed_ = false;
      return true;
    }
    return false;
  }

 private:
  std::size_t window_;
  float max_span_kg_;
  float max_abs_kg_;
  std::uint32_t timeout_ms_;
  bool armed_ = false;
  std::uint32_t armed_at_ = 0;
  float values_[kMaxWindow] = {};
  std::size_t count_ = 0;
  std::size_t head_ = 0;
};

}  // namespace syncfit
