#include "raop/codec.h"

#include <gtest/gtest.h>

#include <cstring>

#include "gtest/gtest.h"
#include "raop/rtp.h"

using namespace AirBeamCore::raop;

TEST(PCMCodecTest, EncodeSwapsBytesCorrectly) {
  PCMCodec codec;

  uint8_t input_data[] = {
      0x11, 0x22, 0x33, 0x44,  // 应变成 22 11 44 33
      0x55, 0x66, 0x77, 0x88   // 应变成 66 55 88 77
  };
  uint8_t output_data[8] = {};

  RtpAudioPacketChunk input;
  memset(input.data_, 0, sizeof(input.data_));
  memcpy(input.data_, input_data, sizeof(input_data));
  input.len_ = sizeof(input_data);

  RtpAudioPacketChunk output;
  memset(output.data_, 0, sizeof(output.data_));
  output.len_ = sizeof(output_data);

  codec.Encode(input, output);

  uint8_t expected[] = {0x22, 0x11, 0x44, 0x33, 0x66, 0x55, 0x88, 0x77};

  for (size_t i = 0; i < sizeof(expected); ++i) {
    EXPECT_EQ(output.data_[i], expected[i]) << "Mismatch at byte " << i;
  }
}

TEST(PCMCodecTest, EncodeIgnoresIncompleteChunkAtEnd) {
  PCMCodec codec;

  uint8_t input_data[] = {0x10, 0x20, 0x30};  // 长度 < 4，应跳过不处理
  uint8_t output_data[3] = {0xff, 0xff, 0xff};

  RtpAudioPacketChunk input;
  memset(input.data_, 0, sizeof(input.data_));
  memcpy(input.data_, input_data, sizeof(input_data));
  input.len_ = sizeof(input_data);

  RtpAudioPacketChunk output;
  memset(output.data_, 0, sizeof(output.data_));
  output.len_ = sizeof(output_data);

  codec.Encode(input, output);

  // 应全是 0
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(output.data_[i], 0) << "Byte " << i << " should be 0";
  }
}

TEST(PCMCodecTest, EncodeHandlesExactMultipleOf4) {
  PCMCodec codec;

  uint8_t input_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  uint8_t output_data[8] = {};

  RtpAudioPacketChunk input;
  memset(input.data_, 0, sizeof(input.data_));
  memcpy(input.data_, input_data, sizeof(input_data));
  input.len_ = sizeof(input_data);

  RtpAudioPacketChunk output;
  memset(output.data_, 0, sizeof(output.data_));
  output.len_ = sizeof(output_data);

  codec.Encode(input, output);

  uint8_t expected[] = {0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07};

  EXPECT_EQ(0, memcmp(expected, output.data_, 8));
}