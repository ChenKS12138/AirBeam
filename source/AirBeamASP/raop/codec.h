#pragma once

#include "rtp.h"

class PCMCodec {
 public:
  static void Encode(const RtpAudioPacketChunk& input,
                     RtpAudioPacketChunk& output);
};