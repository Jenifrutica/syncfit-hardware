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

#include "auto_tare.hpp"

void test_auto_tare_waits_for_rest_and_fires_once() {
  syncfit::AutoTareGate gate(30, 0.08f, 6.0f, 60000);
  std::uint32_t t = 0;
  TEST_ASSERT_FALSE(gate.push(4.7f, t));  // not armed yet
  gate.arm(t);
  // A push in progress (moving and heavy): never tares.
  for (int i = 0; i < 40; ++i) TEST_ASSERT_FALSE(gate.push(8.0f + (i % 5) * 0.4f, t += 100));
  // At rest with a 4.7 kg drift: tares after a full still window, once.
  bool fired = false;
  for (int i = 0; i < 40; ++i) {
    if (gate.push(4.70f + (i % 2) * 0.02f, t += 100)) { TEST_ASSERT_FALSE(fired); fired = true; }
  }
  TEST_ASSERT_TRUE(fired);
  TEST_ASSERT_FALSE(gate.armed());
}

void test_auto_tare_ignores_heavy_static_load_and_times_out() {
  syncfit::AutoTareGate gate(30, 0.08f, 6.0f, 60000);
  std::uint32_t t = 1000;
  gate.arm(t);
  // Someone leaning 12 kg perfectly still: too heavy to be drift.
  for (int i = 0; i < 100; ++i) TEST_ASSERT_FALSE(gate.push(12.0f, t += 100));
  // After the timeout it disarms and keeps the old zero.
  TEST_ASSERT_FALSE(gate.push(0.1f, t += 60000));
  TEST_ASSERT_FALSE(gate.armed());
}
