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
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 74.1f, metrics.heartRateBpm());
}

void test_heart_rate_uses_the_median_not_the_last_interval() {
  syncfit::RrMetrics metrics;
  metrics.addRr(800);
  metrics.addRr(820);
  metrics.addRr(800);
  metrics.addRr(820);
  metrics.addRr(300);  // false peak (200 bpm): must not move the median
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 74.1f, metrics.heartRateBpm());
}

void test_autocorrelation_estimates_a_clean_pulse() {
  float samples[256] = {};
  for (int i = 0; i < 256; ++i) {
    const float t = static_cast<float>(i) / 100.0f;
    samples[i] = std::sin(2.0f * syncfit::kPi * 1.2f * t) +
                 0.2f * std::sin(2.0f * syncfit::kPi * 2.4f * t);
  }
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 72.0f,
                           syncfit::estimateHeartRateBpm(samples, 256, 100));
}

void test_autocorrelation_rejects_a_flat_signal() {
  float samples[256] = {};
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f,
                           syncfit::estimateHeartRateBpm(samples, 256, 100));
}

void test_median_filter_rejects_spikes() {
  syncfit::MedianFilter<21> filter;
  for (int i = 0; i < 21; ++i) filter.push(80.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 80.0f, filter.push(197.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 80.0f, filter.push(40.0f));
}

void test_sample_rate_meter_tracks_the_real_rate() {
  syncfit::SampleRateMeter meter(100.0f, 1000);
  meter.add(0, 0);
  std::uint32_t t = 0;
  // 119 samples per second, delivered 8 at a time.
  std::size_t pending = 0;
  for (int ms = 1; ms <= 5000; ++ms) {
    if ((ms * 119) / 1000 != ((ms - 1) * 119) / 1000) ++pending;
    if (pending >= 8 || ms % 70 == 0) { meter.add(pending, static_cast<std::uint32_t>(ms)); pending = 0; t = ms; }
  }
  (void)t;
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 119.0f, meter.rate());
}

void test_heart_rate_uses_the_measured_sample_rate() {
  // A 86 bpm pulse sampled at 119 Hz: assuming 100 Hz would report ~72 bpm.
  float samples[256];
  for (int i = 0; i < 256; ++i) samples[i] = std::sin(2.0f * 3.14159265f * (86.0f / 60.0f) * (static_cast<float>(i) / 119.0f));
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 86.0f, syncfit::estimateHeartRateBpm(samples, 256, 119.0f));
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 72.3f, syncfit::estimateHeartRateBpm(samples, 256, 100.0f));
}

