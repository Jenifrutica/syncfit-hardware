// Fixed-capacity single-producer / single-consumer queue.
// O(1) push/pop, no heap allocation after construction.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace syncfit {

template <typename T, std::size_t Capacity>
class SpscQueue {
  static_assert(Capacity > 1, "Capacity must leave room for the sentinel slot");

 public:
  SpscQueue() = default;

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  bool push(const T& value) {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    const std::size_t next = (head + 1) % Capacity;
    if (next == tail_.load(std::memory_order_acquire)) return false;
    buffer_[head] = value;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& out) {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) return false;
    out = buffer_[tail];
    tail_.store((tail + 1) % Capacity, std::memory_order_release);
    return true;
  }

  std::size_t size() const {
    const std::size_t head = head_.load(std::memory_order_acquire);
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    return head >= tail ? head - tail : Capacity - tail + head;
  }

  bool empty() const { return size() == 0; }
  bool full() const { return size() == Capacity - 1; }
  static constexpr std::size_t capacity() { return Capacity - 1; }

  void clear() {
    tail_.store(head_.load(std::memory_order_acquire), std::memory_order_release);
  }

 private:
  std::array<T, Capacity> buffer_{};
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

}  // namespace syncfit
