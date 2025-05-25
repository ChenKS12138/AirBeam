// Copyright (c) 2025 ChenKS12138

#include "random.h"

#include <random>

#include "fmt/core.h"

namespace AirBeamCore {
namespace helper {

RandomGenerator::RandomGenerator() {}

RandomGenerator& RandomGenerator::GetInstance() {
  static RandomGenerator instance;
  return instance;
}
uint64_t RandomGenerator::GenU64() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;
  return dis(gen);
}
const std::string RandomGenerator::GenNumStr(int length) {
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<int> dist(0, 9);
  std::string result;
  result.reserve(length);
  for (int i = 0; i < length; ++i) {
    result += fmt::format("{}", dist(gen));
  }
  return result;
}
const std::string RandomGenerator::GenHexStr(int length) {
  static const char hex_chars[] = "0123456789abcdef";
  std::string result;
  result.reserve(length);
  for (int i = 0; i < length; ++i) {
    result += hex_chars[std::rand() % 16];
  }
  return result;
}

}  // namespace helper
}  // namespace AirBeamCore