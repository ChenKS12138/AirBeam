// Copyright (c) 2025 ChenKS12138

#pragma once

#include <cstdint>

namespace AirBeamCore {
namespace helper {
enum ErrCode : uint64_t {
  kOk = 0,
  // common
  kErrUnknown = 1,
  kErrInvalidParam = 2,
  // TCP
  kErrTcpSocketCreate = 65537,
  kErrTcpAddrParse = 65538,
  kErrTcpConnect = 65539,
  kErrTcpSend = 65540,
  kErrTcpRecv = 65541,
  // UDP
  kErrUdpSocketCreate = 131073,
  kErrUdpBind = 131074,
  kErrUdpAddrParse = 131075,
  kErrUdpSend = 131076,
  kErrUdpRecv = 131077,
};
}  // namespace helper
}  // namespace AirBeamCore