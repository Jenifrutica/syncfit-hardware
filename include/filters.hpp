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

// Median over a fixed window: rejects isolated autocorrelation octave errors
// (40/197 bpm) while keeping the real resting rate.
template <std::size_t N>
class MedianFilter {
  static_assert(N > 0 && N % 2 == 1, "MedianFilter needs an odd capacity");

 public:
  float push(float value) {
    buffer_[index_] = value;
    index_ = (index_ + 1) % N;
    if (count_ < N) ++count_;
    return this->value();
  }

  float value() const {
    if (count_ == 0) return 0.0f;
    std::array<float, N> sorted{};
    for (std::size_t i = 0; i < count_; ++i) sorted[i] = buffer_[i];
    std::sort(sorted.begin(), sorted.begin() + count_);
    return sorted[count_ / 2];
  }

  std::size_t count() const { return count_; }

  void reset() {
    buffer_.fill(0.0f);
    index_ = 0;
    count_ = 0;
  }

 private:
  std::array<float, N> buffer_{};
  std::size_t index_ = 0;
  std::size_t count_ = 0;
};

// Adaptive peak detector for the filtered PPG signal. The envelope follows the
// signal amplitude so a weak finger contact does not produce false beats.
class PeakDetector {
 public:
  // 450 ms rejects the dicrotic notch of the same beat (which lands ~300 ms
  // after the systolic peak) while still allowing up to ~133 bpm at rest.
  explicit PeakDetector(std::uint32_t refractory_ms = 450)
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

// Estimate heart rate from a band-passed PPG window with autocorrelation.
// Peak detectors can count the dicrotic notch as a second beat; autocorrelation
// locks onto the dominant repetition period instead, which is much more stable
// at rest. Returns 0 when the window is too short or the correlation is weak.
// `sample_rate_hz` must be the MEASURED rate: the MAX30102 internal clock can
// run several percent off its nominal 100 sps, and the bpm scales with it.
inline float estimateHeartRateBpm(const float* samples, std::size_t count,
                                  float sample_rate_hz = 100.0f) {
  if (samples == nullptr || count < 64 || sample_rate_hz <= 0.0f) return 0.0f;

  float mean = 0.0f;
  for (std::size_t i = 0; i < count; ++i) mean += samples[i];
  mean /= static_cast<float>(count);

  float variance = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    const float centered = samples[i] - mean;
    variance += centered * centered;
  }
  variance /= static_cast<float>(count);
  if (variance <= 1.0e-6f) return 0.0f;

  // The check-in is a resting measurement: 50–130 bpm avoids the 40/197
  // octave errors that a noisy dicrotic notch can create.
  const std::size_t min_lag =
      static_cast<std::size_t>(sample_rate_hz * 60.0f / 130.0f);  // 130 bpm (resting check-in)
  std::size_t max_lag =
      static_cast<std::size_t>(sample_rate_hz * 60.0f / 50.0f);  // 50 bpm
  if (max_lag >= count) max_lag = count - 1;
  if (max_lag <= min_lag + 1) return 0.0f;

  std::array<float, 160> scores{};
  if (max_lag >= scores.size()) max_lag = scores.size() - 1;

  float best_score = 0.0f;
  std::size_t best_lag = 0;
  for (std::size_t lag = min_lag; lag <= max_lag; ++lag) {
    float sum = 0.0f;
    for (std::size_t i = 0; i + lag < count; ++i) {
      sum += (samples[i] - mean) * (samples[i + lag] - mean);
    }
    const float score =
        sum / (static_cast<float>(count - lag) * variance);
    scores[lag] = score;
  }
  // Only a real peak counts: with motion or poor contact the correlation just
  // slides toward one end of the range, and the edge (150 or 45 bpm) is not a
  // pulse. No interior peak -> no reading ("—") instead of a wrong number.
  for (std::size_t lag = min_lag + 1; lag < max_lag; ++lag) {
    const float score = scores[lag];
    if (score >= scores[lag - 1] && score >= scores[lag + 1] && score > best_score) {
      best_score = score;
      best_lag = lag;
    }
  }

  if (best_lag == 0 || best_score < 0.20f) return 0.0f;

  // Octave check: the dicrotic notch makes every beat look like two, so the
  // best lag can be HALF the real period (e.g. 152 bpm for a real 76). If the
  // double lag is almost as similar, that is the true beat.
  const std::size_t double_lag = best_lag * 2;
  if (double_lag <= max_lag && scores[double_lag] >= 0.70f * best_score) {
    std::size_t refined = double_lag;
    for (std::size_t lag = double_lag > min_lag + 2 ? double_lag - 2 : min_lag;
         lag <= double_lag + 2 && lag <= max_lag; ++lag) {
      if (scores[lag] > scores[refined]) refined = lag;
    }
    best_lag = refined;
    best_score = scores[refined];
  }

  // Parabolic interpolation gives sub-sample period resolution.
  const float y0 = best_lag > 0 ? scores[best_lag - 1] : best_score;
  const float y1 = best_score;
  const float y2 = best_lag < max_lag ? scores[best_lag + 1] : best_score;
  const float denominator = y0 - 2.0f * y1 + y2;
  float offset = 0.0f;
  if (std::fabs(denominator) > 1.0e-9f) {
    offset = 0.5f * (y0 - y2) / denominator;
  }
  offset = clampf(offset, -0.5f, 0.5f);

  const float period_samples = static_cast<float>(best_lag) + offset;
  if (period_samples <= 0.0f) return 0.0f;
  return clampf(60.0f * sample_rate_hz / period_samples,
                30.0f, 220.0f);
}

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

  // Median of the last intervals: one false peak or one missed beat no longer
  // throws the displayed heart rate to 24 or 200 bpm.
  float heartRateBpm() const {
    if (count_ == 0) return 0.0f;
    const float rr = static_cast<float>(medianRr());
    return rr > 0.0f ? clampf(60000.0f / rr, 0.0f, 240.0f) : 0.0f;
  }

  std::size_t count() const { return count_; }
  void reset() {
    count_ = 0;
    index_ = 0;
    buffer_.fill(0);
  }

 private:
  std::uint32_t medianRr() const {
    std::array<std::uint32_t, kCapacity> sorted{};
    for (std::size_t i = 0; i < count_; ++i) sorted[i] = at(i);
    std::sort(sorted.begin(), sorted.begin() + count_);
    const std::size_t middle = count_ / 2;
    if (count_ % 2 == 0) {
      return (sorted[middle - 1] + sorted[middle]) / 2;
    }
    return sorted[middle];
  }

  std::uint32_t at(std::size_t logical_index) const {
    const std::size_t start = count_ == kCapacity ? index_ : 0;
    return buffer_[(start + logical_index) % kCapacity];
  }

  std::array<std::uint32_t, kCapacity> buffer_{};
  std::size_t index_ = 0;
  std::size_t count_ = 0;
};

// Measures the real sample rate of a stream: count samples over wall-clock
// windows of `window_ms` and smooth the result. Starts at the nominal rate.
class SampleRateMeter {
 public:
  explicit SampleRateMeter(float nominal_hz = 100.0f, std::uint32_t window_ms = 5000)
      : rate_(nominal_hz), window_ms_(window_ms) {}

  // Returns true when a new window closed (rate() was updated).
  bool add(std::size_t samples, std::uint32_t now_ms) {
    if (!started_) {
      started_ = true;
      start_ms_ = now_ms;
      count_ = 0;
      return false;
    }
    count_ += samples;
    const std::uint32_t elapsed = now_ms - start_ms_;
    if (elapsed < window_ms_) return false;
    const float measured = 1000.0f * static_cast<float>(count_) / static_cast<float>(elapsed);
    // Ignore windows that lost samples (FIFO read stalls) or are absurd.
    if (measured > 40.0f && measured < 450.0f) {
      rate_ = windows_ == 0 ? measured : rate_ * 0.7f + measured * 0.3f;
      ++windows_;
    }
    start_ms_ = now_ms;
    count_ = 0;
    return true;
  }

  float rate() const { return rate_; }
  std::size_t windows() const { return windows_; }

 private:
  float rate_;
  std::uint32_t window_ms_;
  bool started_ = false;
  std::uint32_t start_ms_ = 0;
  std::size_t count_ = 0;
  std::size_t windows_ = 0;
};

}  // namespace syncfit
