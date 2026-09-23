// Fixed-capacity Ring Buffer for the continuous 100 Hz PPG stream.
// O(1) insertion in static memory, avoiding heap fragmentation on the ESP32.

#pragma once

#include <cstddef>

namespace syncfit {

template <typename T, std::size_t Capacity>
class RingBuffer {
 public:
  RingBuffer() = default;

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return Capacity; }
  bool full() const { return size_ == Capacity; }
  bool empty() const { return size_ == 0; }

  // Insert an item in O(1). Returns the overwritten item when full.
  T push(const T& value) {
    T overwritten{};
    const bool was_full = full();
    if (was_full) {
      overwritten = buffer_[head_];
    }
    buffer_[head_] = value;
    head_ = (head_ + 1) % Capacity;
    if (!was_full) {
      ++size_;
    }
    return overwritten;
  }

  const T& at(std::size_t index) const { return buffer_[(start() + index) % Capacity]; }

  const T& latest() const { return buffer_[(head_ + Capacity - 1) % Capacity]; }

  void clear() {
    head_ = 0;
    size_ = 0;
  }

 private:
  std::size_t start() const { return full() ? head_ : 0; }

  T buffer_[Capacity]{};
  std::size_t head_ = 0;
  std::size_t size_ = 0;
};

}  // namespace syncfit
