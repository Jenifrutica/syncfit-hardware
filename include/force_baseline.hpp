// Isometric force baseline and fatigue loss.
//
// The athlete pushes against the base; the first firm push (>= 1.5 kg) arms the set and
// the peak during the capture window becomes the baseline. After that, the
// percentage lost is (baseline - current) / baseline. Releasing the base for a
// moment resets the set so the next capture starts fresh. `START` also resets
// it explicitly, which is what the bench and a future app command use.
//
// Host-testable: no Arduino headers.

#pragma once

#include <algorithm>
#include <cstdint>

namespace syncfit {

class ForceBaseline {
 public:
  static constexpr float kStartKg = 1.5f;
  static constexpr float kReleaseKg = 1.0f;
  static constexpr std::uint32_t kCaptureMs = 2500;
  static constexpr std::uint32_t kReleaseMs = 2000;

  /** Forget the current set; the next push starts a new baseline. */
  void reset() {
    state_ = State::kIdle;
    baseline_kg_ = 0.0f;
    loss_pct_ = 0.0f;
    capture_start_ms_ = 0;
    release_start_ms_ = 0;
  }

  void update(float force_kg, std::uint32_t now_ms) {
    switch (state_) {
      case State::kIdle:
        if (force_kg >= kStartKg) {
          state_ = State::kCapturing;
          baseline_kg_ = force_kg;
          capture_start_ms_ = now_ms;
        }
        break;

      case State::kCapturing:
        baseline_kg_ = std::max(baseline_kg_, force_kg);
        if (now_ms - capture_start_ms_ >= kCaptureMs) {
          state_ = State::kHolding;
          release_start_ms_ = 0;
        }
        break;

      case State::kHolding:
        loss_pct_ = baseline_kg_ > 0.0f
                        ? std::clamp((baseline_kg_ - force_kg) / baseline_kg_ * 100.0f,
                                     0.0f, 100.0f)
                        : 0.0f;
        if (force_kg < kReleaseKg) {
          if (release_start_ms_ == 0) release_start_ms_ = now_ms;
          if (now_ms - release_start_ms_ >= kReleaseMs) reset();
        } else {
          release_start_ms_ = 0;
        }
        break;
    }
  }

  float baselineKg() const { return baseline_kg_; }
  float lossPct() const { return loss_pct_; }
  bool capturing() const { return state_ == State::kCapturing; }
  bool holding() const { return state_ == State::kHolding; }

 private:
  enum class State { kIdle, kCapturing, kHolding };

  State state_ = State::kIdle;
  float baseline_kg_ = 0.0f;
  float loss_pct_ = 0.0f;
  std::uint32_t capture_start_ms_ = 0;
  std::uint32_t release_start_ms_ = 0;
};

}  // namespace syncfit
