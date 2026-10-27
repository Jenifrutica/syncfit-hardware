// Host unit tests for the fixed-capacity SPSC queue.
#include <unity.h>

#include "spsc_queue.hpp"

using Queue = syncfit::SpscQueue<int, 4>;

void test_push_pop_order() {
  Queue queue;
  TEST_ASSERT_TRUE(queue.empty());
  TEST_ASSERT_TRUE(queue.push(1));
  TEST_ASSERT_TRUE(queue.push(2));
  TEST_ASSERT_TRUE(queue.push(3));
  TEST_ASSERT_FALSE(queue.push(4));  // one slot is the sentinel

  int value = 0;
  TEST_ASSERT_TRUE(queue.pop(value));
  TEST_ASSERT_EQUAL_INT(1, value);
  TEST_ASSERT_TRUE(queue.pop(value));
  TEST_ASSERT_EQUAL_INT(2, value);
  TEST_ASSERT_TRUE(queue.pop(value));
  TEST_ASSERT_EQUAL_INT(3, value);
  TEST_ASSERT_FALSE(queue.pop(value));
  TEST_ASSERT_TRUE(queue.empty());
}

void test_queue_clear() {
  Queue queue;
  queue.push(7);
  queue.clear();
  TEST_ASSERT_TRUE(queue.empty());
  int value = 0;
  TEST_ASSERT_FALSE(queue.pop(value));
}
