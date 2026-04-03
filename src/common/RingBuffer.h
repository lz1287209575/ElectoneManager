#pragma once
#include <atomic>
#include <array>
#include <cstddef>
#include <cassert>

namespace em {

/// Lock-free single-producer single-consumer ring buffer.
/// Size must be a power of two.
template<typename T, size_t Capacity>
class RingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "RingBuffer Capacity must be a power of two");
public:
    RingBuffer() : _head(0), _tail(0) {}

    /// Push one item. Returns false if full (non-blocking).
    bool push(const T& item) noexcept {
        const size_t head = _head.load(std::memory_order_relaxed);
        const size_t next = (head + 1) & kMask;
        if (next == _tail.load(std::memory_order_acquire))
            return false; // full
        _data[head] = item;
        _head.store(next, std::memory_order_release);
        return true;
    }

    /// Pop one item. Returns false if empty (non-blocking).
    bool pop(T& item) noexcept {
        const size_t tail = _tail.load(std::memory_order_relaxed);
        if (tail == _head.load(std::memory_order_acquire))
            return false; // empty
        item = _data[tail];
        _tail.store((tail + 1) & kMask, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return _head.load(std::memory_order_acquire) ==
               _tail.load(std::memory_order_acquire);
    }

    size_t size() const noexcept {
        size_t h = _head.load(std::memory_order_acquire);
        size_t t = _tail.load(std::memory_order_acquire);
        return (h - t) & kMask;
    }

    static constexpr size_t capacity() { return Capacity; }

private:
    static constexpr size_t kMask = Capacity - 1;
    std::array<T, Capacity> _data{};
    std::atomic<size_t> _head;
    std::atomic<size_t> _tail;
};

} // namespace em
