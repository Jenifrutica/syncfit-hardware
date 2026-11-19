// Unity runner for all host-side tests.
#include <unity.h>

void test_push_and_size();
void test_wraps_and_keeps_order();
void test_clear();
void test_push_pop_order();
void test_queue_clear();
void test_orders_largest_first();
void test_dc_blocker_removes_constant_offset();
void test_peak_detector_finds_beats();
void test_rmssd_from_rr_intervals();
void test_heart_rate_uses_the_median_not_the_last_interval();
void test_force_baseline_starts_on_a_strong_push();
void test_autocorrelation_estimates_a_clean_pulse();
void test_autocorrelation_rejects_a_flat_signal();
void test_median_filter_rejects_spikes();
void test_force_baseline_resets_after_release();
void test_force_baseline_manual_reset();
void test_builds_valid_contract_shape_and_clamps_values();
void test_rejects_empty_or_oversized_windows();
void test_rejects_small_buffer();
void test_builds_fast_force_message();
void test_force_gate_sends_changes_and_keepalive_only();
void test_parses_http_date_header();
void test_sample_rate_meter_tracks_the_real_rate();
void test_heart_rate_uses_the_measured_sample_rate();
void test_builds_fast_force_message();

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_push_and_size);
  RUN_TEST(test_wraps_and_keeps_order);
  RUN_TEST(test_clear);
  RUN_TEST(test_push_pop_order);
  RUN_TEST(test_queue_clear);
  RUN_TEST(test_orders_largest_first);
  RUN_TEST(test_dc_blocker_removes_constant_offset);
  RUN_TEST(test_peak_detector_finds_beats);
  RUN_TEST(test_rmssd_from_rr_intervals);
  RUN_TEST(test_heart_rate_uses_the_median_not_the_last_interval);
  RUN_TEST(test_force_baseline_starts_on_a_strong_push);
  RUN_TEST(test_autocorrelation_estimates_a_clean_pulse);
  RUN_TEST(test_autocorrelation_rejects_a_flat_signal);
  RUN_TEST(test_median_filter_rejects_spikes);
  RUN_TEST(test_force_baseline_resets_after_release);
  RUN_TEST(test_force_baseline_manual_reset);
  RUN_TEST(test_builds_valid_contract_shape_and_clamps_values);
  RUN_TEST(test_rejects_empty_or_oversized_windows);
  RUN_TEST(test_rejects_small_buffer);
  RUN_TEST(test_builds_fast_force_message);
  RUN_TEST(test_force_gate_sends_changes_and_keepalive_only);
  RUN_TEST(test_parses_http_date_header);
  RUN_TEST(test_sample_rate_meter_tracks_the_real_rate);
  RUN_TEST(test_heart_rate_uses_the_measured_sample_rate);
  return UNITY_END();
}
