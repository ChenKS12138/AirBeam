// Copyright (c) 2025 ChenKS12138

#pragma once

#include <cstdint>
#include <string>

namespace AirBeamCore {
namespace helper {
class RandomGenerator {
 private:
  RandomGenerator();

 public:
  static RandomGenerator& GetInstance();
  uint64_t GenU64();
  const std::string GenNumStr(int length);
  const std::string GenHexStr(int length);
};
}  // namespace helper
}  // namespace AirBeamCore