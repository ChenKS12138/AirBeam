// Copyright (c) 2025 ChenKS12138

#include "rtsp.h"

#include <sstream>

#include "absl/strings/str_split.h"
#include "fmt/core.h"

namespace AirBeamCore {
namespace raop {
std::map<std::string, std::string> ParseKVStr(
    const std::string& content, const std::string& kv_delimiter,
    const std::string& entry_delimiter) {
  std::map<std::string, std::string> result;
  std::vector<std::string> entries = absl::StrSplit(content, entry_delimiter);

  for (const auto& entry : entries) {
    std::vector<std::string> cols =
        absl::StrSplit(entry, absl::MaxSplits(kv_delimiter, 1));
    if (cols.size() == 2) {
      result[cols[0]] = cols[1];
    } else if (cols.size() == 1) {
      result[cols[0]] = "";
    }
  }

  return result;
}

std::string JoinKVStr(const std::map<std::string, std::string>& data,
                      const std::string& kv_delimiter,
                      const std::string& entry_delimiter) {
  std::stringstream ss;
  bool first = true;
  for (const auto& [key, value] : data) {
    if (!first) {
      ss << entry_delimiter;
    }
    first = false;
    ss << key << kv_delimiter << value;
  }
  return ss.str();
}

RtspMessage RtspMessage::Parse(const std::string& content) {
  RtspMessage msg;

  auto it = content.begin();

  while (*it != '\r' && *(it + 1) != '\n') {
    msg.start_line_.push_back(*it);
    it++;
  }
  it += 2;

  while (true) {
    std::string key;
    std::string value;

    if (*it == '\r' && *(it + 1) == '\n') {
      it += 2;
      break;
    }

    while (*it != ':') {
      key.push_back(*it);
      it++;
    }

    it += 2;

    while (*it != '\r' && *(it + 1) != '\n') {
      value.push_back(*it);
      it++;
    }
    it += 2;

    msg.headers_.emplace_back(key, value);
  }

  msg.body_ = std::string(it, content.end());

  return msg;
}
std::string RtspMessage::ToString() const {
  std::stringstream ss;
  ss << start_line_ << "\r\n";
  for (const auto& [key, value] : headers_) {
    ss << key << ": " << value + "\r\n";
  }
  ss << "\r\n";
  ss << body_;
  return ss.str();
}

std::string RtspMessage::GetHeader(const std::string& key) {
  for (const auto& [key_, value] : headers_) {
    if (key_ == key) {
      return value;
    }
  }
  return "";
}

template <>
RtspReqMessage RtspMsgBuilder<RtspReqMessage>::Build() {
  RtspReqMessage message;
  message.start_line_ = fmt::format("{} {} RTSP/1.0", method_, uri_);
  message.headers_ = headers_;
  message.body_ = body_;
  return message;
}

template <>
RtspRespMessage RtspMsgBuilder<RtspRespMessage>::Build() {
  RtspRespMessage message;
  message.start_line_ =
      fmt::format("RTSP/1.0 {} {}", status_code_, status_text_);
  message.headers_ = headers_;
  message.body_ = body_;
  return message;
}
}  // namespace raop
}  // namespace AirBeamCore
