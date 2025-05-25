// Copyright (c) 2025 ChenKS12138

#pragma once

#include <map>
#include <string>
#include <unordered_map>
namespace AirBeamCore {
namespace raop {
std::map<std::string, std::string> ParseKVStr(
    const std::string& content, const std::string& kv_delimiter,
    const std::string& entry_delimiter);

class RtspMessage {
 public:
  static RtspMessage Parse(const std::string& content);

  std::string ToString() const;

  std::string start_line_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};
}  // namespace raop
}  // namespace AirBeamCore
