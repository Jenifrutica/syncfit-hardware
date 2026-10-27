// Fixed-capacity max-heap used to prioritize on-device alerts.
// O(log n) insert/extract, no heap allocation.

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>

namespace syncfit {

template <typename T, std::size_t Capacity, typename Compare = std::less<T>>
class FixedMaxHeap {
 public:
  FixedMaxHeap() = default;

  bool push(const T& value) {
    if (size_ >= Capacity) return false;
    data_[size_] = value;
    ++size_;
    std::push_heap(data_.begin(), data_.begin() + size_, Compare{});
    return true;
  }

  bool pop(T& out) {
    if (size_ == 0) return false;
    std::pop_heap(data_.begin(), data_.begin() + size_, Compare{});
    --size_;
    out = data_[size_];
    return true;
  }

  const T& top() const { return data_[0]; }
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  bool full() const { return size_ == Capacity; }
  static constexpr std::size_t capacity() { return Capacity; }

  void clear() { size_ = 0; }

 private:
  std::array<T, Capacity> data_{};
  std::size_t size_ = 0;
};

}  // namespace syncfit
