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
void test_builds_valid_contract_shape_and_clamps_values();
void test_rejects_empty_or_oversized_windows();
void test_rejects_small_buffer();

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
  RUN_TEST(test_builds_valid_contract_shape_and_clamps_values);
  RUN_TEST(test_rejects_empty_or_oversized_windows);
  RUN_TEST(test_rejects_small_buffer);
  return UNITY_END();
}
