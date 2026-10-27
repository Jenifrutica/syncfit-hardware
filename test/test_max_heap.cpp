// Host unit tests for the fixed-capacity max-heap.
#include <unity.h>

#include "max_heap.hpp"

using Heap = syncfit::FixedMaxHeap<int, 4>;

void test_orders_largest_first() {
  Heap heap;
  TEST_ASSERT_TRUE(heap.push(3));
  TEST_ASSERT_TRUE(heap.push(1));
  TEST_ASSERT_TRUE(heap.push(4));
  TEST_ASSERT_TRUE(heap.push(2));
  TEST_ASSERT_TRUE(heap.full());
  TEST_ASSERT_FALSE(heap.push(5));
  TEST_ASSERT_EQUAL_INT(4, heap.top());

  int value = 0;
  TEST_ASSERT_TRUE(heap.pop(value));
  TEST_ASSERT_EQUAL_INT(4, value);
  TEST_ASSERT_TRUE(heap.pop(value));
  TEST_ASSERT_EQUAL_INT(3, value);
  TEST_ASSERT_TRUE(heap.pop(value));
  TEST_ASSERT_EQUAL_INT(2, value);
  TEST_ASSERT_TRUE(heap.pop(value));
  TEST_ASSERT_EQUAL_INT(1, value);
  TEST_ASSERT_FALSE(heap.pop(value));
}
