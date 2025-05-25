// Copyright (c) 2025 ChenKS12138

#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace AirBeamCore {
namespace raop {
std::map<std::string, std::string> ParseKVStr(
    const std::string& content, const std::string& kv_delimiter,
    const std::string& entry_delimiter);

class RtspMessage {
 public:
  static RtspMessage Parse(const std::string& content);
  std::string ToString() const;
  const std::string& GetStartLine() const { return start_line_; }
  std::string GetHeader(const std::string& key);
  const std::string& GetBody() const { return body_; }

 protected:
  std::string start_line_;
  std::vector<std::tuple<std::string, std::string>> headers_;
  std::string body_;
};

template <typename TRtspMessage>
class RtspMsgBuilder {
 public:
  RtspMsgBuilder<TRtspMessage> SetMethod(const std::string& method) {
    method_ = method;
    return *this;
  }
  RtspMsgBuilder<TRtspMessage> SetUri(const std::string& uri) {
    uri_ = uri;
    return *this;
  }
  RtspMsgBuilder<TRtspMessage> SetStatusCode(uint32_t status_code) {
    status_code_ = status_code;
    return *this;
  }
  RtspMsgBuilder<TRtspMessage> SetStatusText(const std::string& status_text) {
    status_text_ = status_text;
    return *this;
  }
  RtspMsgBuilder<TRtspMessage> AddHeader(const std::string& key,
                                         const std::string& value) {
    headers_.emplace_back(key, value);
    return *this;
  }
  RtspMsgBuilder<TRtspMessage> SetBody(const std::string& body) {
    body_ = body;
    return *this;
  }
  TRtspMessage Build() = delete;

 protected:
  std::string method_;
  std::string uri_;
  uint32_t status_code_;
  std::string status_text_;
  std::vector<std::tuple<std::string, std::string>> headers_;
  std::string body_;
};

class RtspReqMessage : public RtspMessage {
 public:
  friend class RtspMsgBuilder<RtspReqMessage>;
};

class RtspRespMessage : public RtspMessage {
 public:
  friend class RtspMsgBuilder<RtspRespMessage>;
};

template <>
RtspReqMessage RtspMsgBuilder<RtspReqMessage>::Build();

template <>
RtspRespMessage RtspMsgBuilder<RtspRespMessage>::Build();

}  // namespace raop
}  // namespace AirBeamCore
