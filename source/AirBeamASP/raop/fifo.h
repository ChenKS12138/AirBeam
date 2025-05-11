// Copyright (c) 2025 ChenKS12138

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

class ConcurrentByteFIFO {
 public:
  explicit ConcurrentByteFIFO(size_t capacity)
      : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), size_(0) {}

  size_t Write(const uint8_t* data, size_t length);

  size_t Read(uint8_t* data, size_t length);

  bool Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_ == 0;
  }

  bool Full() const;

 private:
  std::vector<uint8_t> buffer_;
  const size_t capacity_;
  size_t head_;
  size_t tail_;
  size_t size_;

  mutable std::mutex mutex_;
  std::condition_variable not_full_cv_;
  std::condition_variable not_empty_cv_;
};
