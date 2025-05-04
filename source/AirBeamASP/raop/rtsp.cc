#include "rtsp.h"

#include <sstream>

#include "absl/strings/str_split.h"

std::map<std::string, std::string> ParseKVStr(
    const std::string& content, const std::string& kv_delimiter,
    const std::string& entry_delimiter) {
  std::map<std::string, std::string> result;
  std::vector<std::string> entries = absl::StrSplit(content, entry_delimiter);

  for (const auto& entry : entries) {
    std::vector<std::string> cols =
        absl::StrSplit(entry, absl::MaxSplits(kv_delimiter, 1));
    result[cols[0]] = cols[1];
  }

  return result;
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

    msg.headers_[key] = value;
  }

  msg.body_ = std::string(it, content.end());

  return msg;
}

std::string RtspMessage::ToString() const {
  std::stringstream ss;
  ss << "start_line:" << std::endl << start_line_ << std::endl;
  ss << "headers:" << std::endl;
  for (const auto& header : headers_) {
    ss << header.first << ": " << header.second << std::endl;
  }
  ss << "body:" << std::endl << body_ << std::endl;
  return ss.str();
}