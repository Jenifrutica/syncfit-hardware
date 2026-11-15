#include <unity.h>

#include "force_baseline.hpp"

void test_force_baseline_starts_on_a_strong_push() {
  syncfit::ForceBaseline baseline;
  baseline.update(1.4f, 0);
  TEST_ASSERT_FALSE(baseline.capturing());

  baseline.update(1.6f, 100);
  TEST_ASSERT_TRUE(baseline.capturing());
  baseline.update(6.5f, 1200);
  baseline.update(6.5f, 2600);  // capture window closed
  TEST_ASSERT_TRUE(baseline.holding());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.5f, baseline.baselineKg());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, baseline.lossPct());

  baseline.update(4.875f, 3000);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 25.0f, baseline.lossPct());
}

void test_force_baseline_resets_after_release() {
  syncfit::ForceBaseline baseline;
  baseline.update(7.0f, 0);
  baseline.update(7.0f, 2500);
  baseline.update(3.5f, 3000);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 50.0f, baseline.lossPct());

  baseline.update(0.5f, 4000);
  baseline.update(0.5f, 6000);  // two seconds below the release threshold
  TEST_ASSERT_FALSE(baseline.holding());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, baseline.lossPct());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, baseline.baselineKg());
}

void test_force_baseline_manual_reset() {
  syncfit::ForceBaseline baseline;
  baseline.update(6.0f, 0);
  baseline.update(6.0f, 2500);
  baseline.update(4.0f, 3000);
  TEST_ASSERT_TRUE(baseline.lossPct() > 0.0f);

  baseline.reset();
  TEST_ASSERT_FALSE(baseline.holding());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, baseline.lossPct());
}
