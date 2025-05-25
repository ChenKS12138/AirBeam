// Copyright (c) 2025 ChenKS12138

#include "rtsp_client.h"

#include "helper/errcode.h"
#include "helper/network.h"
#include "raop/rtsp.h"

namespace AirBeamCore {
namespace raop {
int RTSPClient::DoRequest(const RtspReqMessage& request,
                          RtspRespMessage& response) {
  int ret = helper::TCPClient::Write(request.ToString());
  if (ret < 0) {
    return ret;
  }

  std::string buffer;
  ret = helper::TCPClient::Read(buffer);
  if (ret < 0) {
    return ret;
  }
  auto response_msg = RtspMessage::Parse(buffer);
  response = *static_cast<RtspRespMessage*>(&response_msg);

  return helper::kOk;
}
}  // namespace raop
}  // namespace AirBeamCore