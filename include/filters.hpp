// Small, allocation-free signal-processing blocks for the PPG stream.
// These are host-testable: no Arduino headers here.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace syncfit {

constexpr float kPi = 3.14159265358979323846f;

inline float clampf(float value, float low, float high) {
  return std::max(low, std::min(high, value));
}

// First-order DC blocker: removes the large optical offset and leaves the
// pulsatile component. y[n] = x[n] - x[n-1] + a*y[n-1].
class DcBlocker {
 public:
  explicit DcBlocker(float alpha = 0.995f) : alpha_(alpha) {}

  float process(float x) {
    const float y = x - previous_x_ + alpha_ * previous_y_;
    previous_x_ = x;
    previous_y_ = y;
    return y;
  }

  void reset() {
    previous_x_ = 0.0f;
    previous_y_ = 0.0f;
  }

 private:
  float alpha_;
  float previous_x_ = 0.0f;
  float previous_y_ = 0.0f;
};

template <std::size_t N>
class MovingAverage {
  static_assert(N > 0, "MovingAverage needs at least one sample");

 public:
  float push(float value) {
    sum_ -= buffer_[index_];
    buffer_[index_] = value;
    sum_ += value;
    index_ = (index_ + 1) % N;
    if (count_ < N) ++count_;
    return sum_ / static_cast<float>(count_);
  }

  float value() const { return count_ == 0 ? 0.0f : sum_ / static_cast<float>(count_); }
  void reset() {
    buffer_.fill(0.0f);
    sum_ = 0.0f;
    index_ = 0;
    count_ = 0;
  }

 private:
  std::array<float, N> buffer_{};
  float sum_ = 0.0f;
  std::size_t index_ = 0;
  std::size_t count_ = 0;
};

// Adaptive peak detector for the filtered PPG signal. The envelope follows the
// signal amplitude so a weak finger contact does not produce false beats.
class PeakDetector {
 public:
  explicit PeakDetector(std::uint32_t refractory_ms = 300)
      : refractory_ms_(refractory_ms) {}

  bool update(float x, std::uint32_t now_ms) {
    const float magnitude = std::fabs(x);
    envelope_ = 0.98f * envelope_ + 0.02f * magnitude;

    if (!initialized_) {
      initialized_ = true;
      previous_ = x;
      return false;
    }

    const float threshold = std::max(0.35f * envelope_, 1.0e-4f);
    const bool falling = x < previous_;
    const bool above = previous_ > threshold;
    const bool refractory_done =
        last_peak_ms_ == 0 || (now_ms - last_peak_ms_) >= refractory_ms_;

    previous_ = x;
    if (falling && above && refractory_done) {
      last_peak_ms_ = now_ms;
      return true;
    }
    return false;
  }

  void reset() {
    initialized_ = false;
    previous_ = 0.0f;
    envelope_ = 0.0f;
    last_peak_ms_ = 0;
  }

  std::uint32_t lastPeakMs() const { return last_peak_ms_; }

 private:
  std::uint32_t refractory_ms_;
  float previous_ = 0.0f;
  float envelope_ = 0.0f;
  std::uint32_t last_peak_ms_ = 0;
  bool initialized_ = false;
};

// RR-interval metrics over a short fixed window. RMSSD is reported in ms.
class RrMetrics {
 public:
  static constexpr std::size_t kCapacity = 32;

  void addRr(std::uint32_t rr_ms) {
    if (rr_ms < 250 || rr_ms > 2500) return;  // 24–240 bpm
    buffer_[index_] = rr_ms;
    index_ = (index_ + 1) % kCapacity;
    if (count_ < kCapacity) ++count_;
  }

  float rmssdMs() const {
    if (count_ < 4) return 0.0f;
    float sum = 0.0f;
    std::size_t pairs = 0;
    for (std::size_t i = 1; i < count_; ++i) {
      const float diff = static_cast<float>(at(i)) - static_cast<float>(at(i - 1));
      sum += diff * diff;
      ++pairs;
    }
    if (pairs == 0) return 0.0f;
    return clampf(std::sqrt(sum / static_cast<float>(pairs)), 0.0f, 300.0f);
  }

  float heartRateBpm() const {
    if (count_ == 0) return 0.0f;
    const float rr = static_cast<float>(at(count_ - 1));
    return rr > 0.0f ? clampf(60000.0f / rr, 0.0f, 240.0f) : 0.0f;
  }

  std::size_t count() const { return count_; }
  void reset() {
    count_ = 0;
    index_ = 0;
    buffer_.fill(0);
  }

 private:
  std::uint32_t at(std::size_t logical_index) const {
    const std::size_t start = count_ == kCapacity ? index_ : 0;
    return buffer_[(start + logical_index) % kCapacity];
  }

  std::array<std::uint32_t, kCapacity> buffer_{};
  std::size_t index_ = 0;
  std::size_t count_ = 0;
};

}  // namespace syncfit
