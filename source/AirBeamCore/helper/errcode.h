// Copyright (c) 2025 ChenKS12138

#pragma once

#include <cstdint>

namespace AirBeamCore {
namespace helper {
// 错误码分区：
// 通用: 0x0000_0000 ~ 0x0000_0FFF
// TCP:  0x0001_0000 ~ 0x0001_FFFF
// UDP:  0x0002_0000 ~ 0x0002_FFFF

#define ERR_CODE_GEN(base, name, val) name = ((base) + (val))

enum ErrCode : uint64_t {
  kSuccess = 0,
  // 通用错误码
  ERR_CODE_GEN(0x00000000, kErrUnknown, 1),
  ERR_CODE_GEN(0x00000000, kErrInvalidParam, 2),
  // TCP 错误码
  ERR_CODE_GEN(0x00010000, kErrTcpSocketCreate, 1),
  ERR_CODE_GEN(0x00010000, kErrTcpAddrParse, 2),
  ERR_CODE_GEN(0x00010000, kErrTcpConnect, 3),
  ERR_CODE_GEN(0x00010000, kErrTcpSend, 4),
  ERR_CODE_GEN(0x00010000, kErrTcpRecv, 5),
  // UDP 错误码
  ERR_CODE_GEN(0x00020000, kErrUdpSocketCreate, 1),
  ERR_CODE_GEN(0x00020000, kErrUdpBind, 2),
  ERR_CODE_GEN(0x00020000, kErrUdpAddrParse, 3),
  ERR_CODE_GEN(0x00020000, kErrUdpSend, 4),
  ERR_CODE_GEN(0x00020000, kErrUdpRecv, 5),
};
}  // namespace helper
}  // namespace AirBeamCore