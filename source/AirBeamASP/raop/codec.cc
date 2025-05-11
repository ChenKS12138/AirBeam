// Copyright (c) 2025 ChenKS12138

#include "codec.h"

#ifdef SIMD_ARM
#include <arm_neon.h>

void PCMCodec::Encode(const RtpAudioPacketChunk& input,
                      RtpAudioPacketChunk& output) {
  size_t len = input.len_;
  const uint8_t* in = input.data_;
  uint8_t* out = output.data_;
  memset(out, 0, len);

  size_t offset = 0;

  for (; offset + 16 <= len; offset += 16) {
    uint8x16_t v = vld1q_u8(in + offset);
    uint8x16_t r = vrev32q_u8(v);
    vst1q_u8(out + offset, r);
  }

  for (; offset + 3 < len; offset += 4) {
    out[offset] = in[offset + 1];
    out[offset + 1] = in[offset];
    out[offset + 2] = in[offset + 3];
    out[offset + 3] = in[offset + 2];
  }
}

#else

void PCMCodec::Encode(const RtpAudioPacketChunk& input,
                      RtpAudioPacketChunk& output) {
  size_t len = input.len_;
  memset(output.data_, 0, sizeof(output.data_));

  for (size_t offset = 0; offset < len; offset += 4) {
    if (offset + 3 < len) {
      output.data_[offset] = input.data_[offset + 1];
      output.data_[offset + 1] = input.data_[offset];
      output.data_[offset + 2] = input.data_[offset + 3];
      output.data_[offset + 3] = input.data_[offset + 2];
    }
  }
}

#endif
