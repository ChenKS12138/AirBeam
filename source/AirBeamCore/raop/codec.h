// Copyright (c) 2025 ChenKS12138

#pragma once

#include "raop/rtp.h"

namespace AirBeamCore {

namespace raop {
class PCMCodec {
 public:
  static void Encode(const RtpAudioPacketChunk& input,
                     RtpAudioPacketChunk& output);
};
}  // namespace raop
}  // namespace AirBeamCore
