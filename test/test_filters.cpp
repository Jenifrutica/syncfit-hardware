// Host unit tests for the PPG filters and RR metrics.
#include <unity.h>

#include <cmath>

#include "filters.hpp"

void test_dc_blocker_removes_constant_offset() {
  syncfit::DcBlocker blocker;
  float output = 0.0f;
  for (int i = 0; i < 5000; ++i) {
    output = blocker.process(1000.0f);
  }
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, output);
}

void test_peak_detector_finds_beats() {
  syncfit::PeakDetector detector;
  int peaks = 0;
  for (int i = 0; i < 1200; ++i) {
    const float t = static_cast<float>(i) / 100.0f;
    const float sample = std::sin(2.0f * syncfit::kPi * 1.2f * t);
    if (detector.update(sample, static_cast<std::uint32_t>(i * 10))) ++peaks;
  }
  TEST_ASSERT_GREATER_THAN_INT(10, peaks);
}

void test_rmssd_from_rr_intervals() {
  syncfit::RrMetrics metrics;
  metrics.addRr(800);
  metrics.addRr(820);
  metrics.addRr(800);
  metrics.addRr(820);
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 20.0f, metrics.rmssdMs());
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 73.2f, metrics.heartRateBpm());
}
