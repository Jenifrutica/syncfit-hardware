// Host unit tests for the Ring Buffer (run with `pio test -e native`).
#include <unity.h>

#include "ring_buffer.hpp"

using IntRing = syncfit::RingBuffer<int, 3>;

void test_push_and_size() {
  IntRing ring;
  TEST_ASSERT_TRUE(ring.empty());
  ring.push(1);
  ring.push(2);
  TEST_ASSERT_EQUAL_UINT(2, ring.size());
  TEST_ASSERT_FALSE(ring.full());
}

void test_wraps_and_keeps_order() {
  IntRing ring;
  ring.push(1);
  ring.push(2);
  ring.push(3);
  TEST_ASSERT_TRUE(ring.full());
  const int overwritten = ring.push(4);
  TEST_ASSERT_EQUAL_INT(1, overwritten);
  TEST_ASSERT_EQUAL_INT(2, ring.at(0));
  TEST_ASSERT_EQUAL_INT(3, ring.at(1));
  TEST_ASSERT_EQUAL_INT(4, ring.at(2));
  TEST_ASSERT_EQUAL_INT(4, ring.latest());
}

void test_clear() {
  IntRing ring;
  ring.push(7);
  ring.clear();
  TEST_ASSERT_TRUE(ring.empty());
  TEST_ASSERT_EQUAL_UINT(0, ring.size());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_push_and_size);
  RUN_TEST(test_wraps_and_keeps_order);
  RUN_TEST(test_clear);
  return UNITY_END();
}
