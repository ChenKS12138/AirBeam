// Copyright (c) 2025 ChenKS12138

#pragma once

#include "rtp.h"

class PCMCodec {
 public:
  static void Encode(const RtpAudioPacketChunk& input,
                     RtpAudioPacketChunk& output);
};
