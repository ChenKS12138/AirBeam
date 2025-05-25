// Copyright (c) 2025 ChenKS12138

#include "fifo.h"

namespace AirBeamCore {
namespace raop {
size_t ConcurrentByteFIFO::Write(const uint8_t* data, size_t length) {
  size_t written = 0;
  while (written < length) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_full_cv_.wait(lock, [this]() { return size_ < capacity_; });
    buffer_[head_] = data[written++];
    head_ = (head_ + 1) % capacity_;
    ++size_;
    lock.unlock();
    not_empty_cv_.notify_one();
  }
  return written;
}

size_t ConcurrentByteFIFO::Read(uint8_t* data, size_t length) {
  size_t read_count = 0;
  while (read_count < length) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_cv_.wait(lock, [this]() { return size_ > 0; });
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
