#include "rtp.h"

#include <sys/types.h>

#include <chrono>
#include <cstdint>
#include <string>

#include "constants.h"
#include "fmt/core.h"

#ifdef SIMD_ARM
#include <arm_neon.h>

static uint16_t read_be16(const uint8_t* data) {
  return __builtin_bswap16(*(uint16_t*)data);
}

static uint32_t read_be32(const uint8_t* data) {
  return __builtin_bswap32(*(uint32_t*)data);
}

static void write_be16(uint8_t* data, uint16_t value) {
  value = __builtin_bswap16(value);
  *(uint16_t*)data = value;
}

static void write_be32(uint8_t* data, uint32_t value) {
  value = __builtin_bswap32(value);
  *(uint32_t*)data = value;
}

static void write_be64(uint8_t* data, uint64_t value) {
  value = __builtin_bswap64(value);
  *(uint64_t*)data = value;
}

#else
static uint16_t read_be16(const uint8_t* data) {
  return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
}

static uint32_t read_be32(const uint8_t* data) {
  return (static_cast<uint32_t>(data[0]) << 24) |
         (static_cast<uint32_t>(data[1]) << 16) |
         (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

static void write_be16(uint8_t* data, uint16_t value) {
  data[0] = static_cast<uint8_t>((value >> 8) & 0xff);
  data[1] = static_cast<uint8_t>(value & 0xff);
}

static void write_be32(uint8_t* data, uint32_t value) {
  data[0] = static_cast<uint8_t>(value >> 24) & 0xff;
  data[1] = static_cast<uint8_t>(value >> 16) & 0xff;
  data[2] = static_cast<uint8_t>(value >> 8) & 0xff;
  data[3] = static_cast<uint8_t>(value) & 0xff;
}

static void write_be64(uint8_t* data, uint64_t value) {
  data[0] = static_cast<uint8_t>((value >> 56) & 0xff);
  data[1] = static_cast<uint8_t>((value >> 48) & 0xff);
  data[2] = static_cast<uint8_t>((value >> 40) & 0xff);
  data[3] = static_cast<uint8_t>((value >> 32) & 0xff);
  data[4] = static_cast<uint8_t>((value >> 24) & 0xff);
  data[5] = static_cast<uint8_t>((value >> 16) & 0xff);
  data[6] = static_cast<uint8_t>((value >> 8) & 0xff);
  data[7] = static_cast<uint8_t>(value & 0xff);
}
#endif

RtpHeader RtpHeader::Deserialize(const uint8_t* data, size_t size) {
  RtpHeader header;
  header.proto = data[0];
  header.type = data[1];
  header.seq = read_be16(data + 2);
  return header;
}

void RtpHeader::Serialize(uint8_t* data) const {
  data[0] = static_cast<uint8_t>(proto);
  data[1] = static_cast<uint8_t>(type);
  write_be16(data + 2, seq);
}

NtpTime NtpTime::Deserialize(const uint8_t* data, size_t size) {
  NtpTime ntp_time;
  ntp_time.seconds = read_be32(data);
  ntp_time.fraction = read_be32(data + 4);
  return ntp_time;
}
void NtpTime::Serialize(uint8_t* data) const {
  write_be32(data, seconds);
  write_be32(data + 4, fraction);
}

NtpTime NtpTime::Now() {
  using namespace std::chrono;
  auto unix_time = std::chrono::system_clock::now().time_since_epoch();
  auto seconds_since_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(unix_time).count();
  auto micros_since_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(unix_time).count() %
      1'000'000;

  return NtpTime{
      static_cast<uint32_t>(seconds_since_epoch + 0x83AA7E80),
      static_cast<uint32_t>((static_cast<uint64_t>(micros_since_epoch) << 32) /
                            1'000'000)};
}

uint64_t NtpTime::IntoTimestamp(uint64_t sample_rate) const {
  uint64_t ntp =
      (static_cast<uint64_t>(seconds) << 32) | static_cast<uint64_t>(fraction);

  return ((ntp >> 16) * sample_rate) >> 16;
}

NtpTime NtpTime::FromTimestamp(uint64_t ts, uint64_t sample_rate) {
  uint64_t ntp = (ts << 16) / (sample_rate << 16);
  NtpTime result;

  result.seconds = static_cast<uint32_t>((ntp >> 32));
  result.fraction = static_cast<uint32_t>(ntp & 0xffffffff);

  return result;
}

std::string NtpTime::ToString() const {
  return fmt::format("{}.{}", seconds, fraction);
}

RtpTimePacket RtpTimePacket::Deserialize(const uint8_t* data, size_t size) {
  RtpTimePacket packet;

  packet.header = RtpHeader::Deserialize(data, size);
  packet.dummy = read_be32(data + 4);
  packet.ref_time = NtpTime::Deserialize(data + 8, 8);
  packet.recv_time = NtpTime::Deserialize(data + 16, 8);
  packet.send_time = NtpTime::Deserialize(data + 24, 8);

  return packet;
}
void RtpTimePacket::Serialize(uint8_t* data) const {
  header.Serialize(data);
  write_be32(data + 4, dummy);
  ref_time.Serialize(data + 8);
  recv_time.Serialize(data + 16);
  send_time.Serialize(data + 24);
}

std::string RtpTimePacket::ToString() const {
  return fmt::format(
      "proto: {}, type: {}, seq: {}, dummy: {}, ref_time: {}, "
      "recv_time: {}, send_time: {}",
      header.proto, header.type, header.seq, dummy, ref_time.ToString(),
      recv_time.ToString(), send_time.ToString());
}

RtpLostPacket RtpLostPacket::Deserialize(const uint8_t* data, size_t size) {
  RtpLostPacket packet;
  packet.header = RtpHeader::Deserialize(data, size);
  packet.seq_number = read_be16(data + 4);
  packet.n = read_be16(data + 6);
  return packet;
}

std::string RtpLostPacket::ToString() const {
  return fmt::format("proto: {}, type: {}, seq: {}, seq_number: {}, n: {}",
                     header.proto, header.type, header.seq, seq_number, n);
}

RtpSyncPacket RtpSyncPacket::Build(uint64_t timestamp, uint64_t sample_rate,
                                   uint64_t latency, bool first) {
  RtpSyncPacket pkt;
  pkt.header.proto = 0x80 | (first ? 0x10 : 0x00);
  pkt.header.type = 0x54 | 0x80;
  pkt.header.seq = 7;
  pkt.curr_time = NtpTime::FromTimestamp(timestamp, kSampleRate44100);

  pkt.rtp_timestamp = static_cast<uint32_t>(timestamp);
  pkt.rtp_timestamp_latency =
      latency > timestamp ? 0 : static_cast<uint32_t>(timestamp - latency);

  return pkt;
}

std::string RtpSyncPacket::ToString() const {
  return fmt::format(
      "proto: {}, type: {}, seq: {}, rtp_timestamp_latency: {}, "
      "curr_time: {}, rtp_timestamp: {}",
      header.proto, header.type, header.seq, rtp_timestamp_latency,
      curr_time.ToString(), rtp_timestamp);
}

void RtpSyncPacket::Serialize(uint8_t* data) const {
  header.Serialize(data);
  write_be32(data + 4, rtp_timestamp_latency);
  curr_time.Serialize(data + 8);
  write_be32(data + 16, rtp_timestamp);
}

void RtpAudioPacket::Serialize(std::vector<uint8_t>& buffer) const {
  buffer.clear();
  buffer.resize(12, 0);
  header.Serialize(buffer.data());
  write_be32(buffer.data() + 4, timestamp);
  write_be32(buffer.data() + 8, ssrc);
  buffer.insert(buffer.end(), data.data_, data.data_ + data.len_);
}

Volume Volume::FromPercent(uint8_t percent) {
  constexpr float kMinVolume = -30.0;
  constexpr float kMaxVolume = 0.0;

  if (percent == 0) {
    return Volume(-144.0);
  }
  if (percent < 100) {
    float volume = kMinVolume + (kMaxVolume - kMinVolume) * percent / 100.0;
    return Volume(volume);
  }
  return Volume(kMaxVolume);
}
