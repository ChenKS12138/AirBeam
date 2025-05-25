// Copyright (c) 2025 ChenKS12138

#pragma once

#include "helper/network.h"
#include "rtsp.h"

namespace AirBeamCore {
namespace raop {
class RTSPClient : public helper::TCPClient {
 public:
  int DoRequest(const RtspReqMessage& request, RtspRespMessage& response);
};
}  // namespace raop
}  // namespace AirBeamCore