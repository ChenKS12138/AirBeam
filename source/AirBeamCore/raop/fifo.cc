// Copyright (c) 2025 ChenKS12138

#include "fifo.h"

namespace AirBeamCore {
namespace raop {
size_t ConcurrentByteFIFO::Write(const uint8_t* data, size_t length,
                                 std::chrono::milliseconds timeout) {
  size_t written = 0;
  while (written < length) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool timeout_occurred = false;
    if (size_ == capacity_) {
      if (timeout == std::chrono::milliseconds(0)) {
        not_full_cv_.wait(lock, [this]() { return size_ < capacity_; });
      } else {
        timeout_occurred = !not_full_cv_.wait_for(
            lock, timeout, [this]() { return size_ < capacity_; });
      }
    }

    if (timeout_occurred) {
      break;
    }

    buffer_[head_] = data[written++];
    head_ = (head_ + 1) % capacity_;
    ++size_;
    lock.unlock();
    not_empty_cv_.notify_one();
  }
  return written;
}

size_t ConcurrentByteFIFO::Read(uint8_t* data, size_t length,
                                std::chrono::milliseconds timeout) {
  size_t read_count = 0;
  while (read_count < length) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool timeout_occurred = false;
    if (size_ == 0) {
      if (timeout == std::chrono::milliseconds(0)) {
        not_empty_cv_.wait(lock, [this]() { return size_ > 0; });
      } else {
        timeout_occurred = !not_empty_cv_.wait_for(
            lock, timeout, [this]() { return size_ > 0; });
      }
    }

    if (timeout_occurred) {
      break;
    }

    data[read_count++] = buffer_[tail_];
    tail_ = (tail_ + 1) % capacity_;
    --size_;
    lock.unlock();
    not_full_cv_.notify_one();
  }
  return read_count;
}

bool ConcurrentByteFIFO::Full() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return size_ == capacity_;
}
}  // namespace raop
}  // namespace AirBeamCore
