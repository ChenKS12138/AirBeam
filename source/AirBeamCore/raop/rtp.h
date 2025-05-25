// Copyright (c) 2025 ChenKS12138

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "constants.h"

struct RtpHeader {
  uint8_t proto;
  uint8_t type;
  uint16_t seq;
  static RtpHeader Deserialize(const uint8_t* data, size_t size);
  void Serialize(uint8_t* data) const;
};

struct NtpTime {
  uint32_t seconds;
  uint32_t fraction;
  static NtpTime Deserialize(const uint8_t* data, size_t size);
  static NtpTime Now();
  static NtpTime FromTimestamp(uint64_t ts, uint64_t sample_rate);
  std::string ToString() const;
  void Serialize(uint8_t* data) const;

  uint64_t IntoTimestamp(uint64_t sample_rate) const;
};

struct RtpTimePacket {
  RtpHeader header;
  uint32_t dummy;
  NtpTime ref_time;
  NtpTime recv_time;
  NtpTime send_time;

  static RtpTimePacket Deserialize(const uint8_t* data, size_t size);
  std::string ToString() const;
  void Serialize(uint8_t* data) const;
};

struct RtpLostPacket {
 public:
  RtpHeader header;
  uint16_t seq_number;
  uint16_t n;

  static RtpLostPacket Deserialize(const uint8_t* data, size_t size);

  std::string ToString() const;
};

struct RtpSyncPacket {
  RtpHeader header;
  uint32_t rtp_timestamp_latency;
  NtpTime curr_time;
  uint32_t rtp_timestamp;

  static RtpSyncPacket Build(uint64_t timestamp, uint64_t sample_rate,
                             uint64_t latency, bool first);

  std::string ToString() const;
  void Serialize(uint8_t* data) const;
};

struct RtpAudioPacketChunk {
  uint8_t data_[kPCMChunkLength * 4];
  size_t len_;
};

struct RtpAudioPacket {
  RtpHeader header;
  uint32_t timestamp;
  uint32_t ssrc;
  RtpAudioPacketChunk data;

  void Serialize(std::vector<uint8_t>& data) const;
};

struct Volume {
 public:
  static Volume FromPercent(uint8_t percent);
  float GetValue() const { return value_; }

 private:
  explicit Volume(float value) : value_(value) {}
  float value_;
};
