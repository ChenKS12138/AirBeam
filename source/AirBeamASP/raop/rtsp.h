#pragma once

#include <map>
#include <string>
#include <unordered_map>

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