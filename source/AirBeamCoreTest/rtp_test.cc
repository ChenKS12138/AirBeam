#include "raop/rtp.h"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using namespace AirBeamCore::raop;

TEST(RtpHeaderTest, SerializeDeserialize) {
  RtpHeader header{0x80, 0x60, 0x1234};
  uint8_t buf[4] = {0};
  header.Serialize(buf);
  RtpHeader header2 = RtpHeader::Deserialize(buf, 4);
  EXPECT_EQ(header.proto, header2.proto);
  EXPECT_EQ(header.type, header2.type);
  EXPECT_EQ(header.seq, header2.seq);
}

TEST(NtpTimeTest, SerializeDeserialize) {
  NtpTime t1{0x12345678, 0x9abcdef0};
  uint8_t buf[8] = {0};
  t1.Serialize(buf);
  NtpTime t2 = NtpTime::Deserialize(buf, 8);
  EXPECT_EQ(t1.seconds, t2.seconds);
  EXPECT_EQ(t1.fraction, t2.fraction);
}

TEST(NtpTimeTest, NowAndToString) {
  NtpTime now = NtpTime::Now();
  std::string s = now.ToString();
  EXPECT_FALSE(s.empty());
}

// TODO(cattchen) fix test
// TEST(NtpTimeTest, FromAndIntoTimestamp) {
//   uint64_t ts = 44100 * 10;
//   uint64_t sr = 44100;
//   NtpTime ntp = NtpTime::FromTimestamp(ts, sr);
//   uint64_t ts2 = ntp.IntoTimestamp(sr);
//   EXPECT_NEAR(ts, ts2, 2);
// }

TEST(RtpTimePacketTest, SerializeDeserialize) {
  RtpTimePacket pkt;
  pkt.header = {0x80, 0x60, 0x1234};
  pkt.dummy = 0xdeadbeef;
  pkt.ref_time = {1, 2};
  pkt.recv_time = {3, 4};
  pkt.send_time = {5, 6};
  uint8_t buf[32] = {0};
  pkt.Serialize(buf);
  RtpTimePacket pkt2 = RtpTimePacket::Deserialize(buf, 32);
  EXPECT_EQ(pkt.header.proto, pkt2.header.proto);
  EXPECT_EQ(pkt.header.type, pkt2.header.type);
  EXPECT_EQ(pkt.header.seq, pkt2.header.seq);
  EXPECT_EQ(pkt.dummy, pkt2.dummy);
  EXPECT_EQ(pkt.ref_time.seconds, pkt2.ref_time.seconds);
  EXPECT_EQ(pkt.recv_time.fraction, pkt2.recv_time.fraction);
  EXPECT_EQ(pkt.send_time.seconds, pkt2.send_time.seconds);
}

TEST(RtpTimePacketTest, ToString) {
  RtpTimePacket pkt;
  pkt.header = {0x80, 0x60, 0x1234};
  pkt.dummy = 0xdeadbeef;
  pkt.ref_time = {1, 2};
  pkt.recv_time = {3, 4};
  pkt.send_time = {5, 6};
  std::string s = pkt.ToString();
  EXPECT_NE(s.find("proto"), std::string::npos);
}

TEST(RtpLostPacketTest, SerializeDeserialize) {
  uint8_t buf[8] = {0x80, 0x60, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};
  RtpLostPacket pkt = RtpLostPacket::Deserialize(buf, 8);
  EXPECT_EQ(pkt.header.proto, 0x80);
  EXPECT_EQ(pkt.header.type, 0x60);
  EXPECT_EQ(pkt.header.seq, 0x1234);
  EXPECT_EQ(pkt.seq_number, 0x5678);
  EXPECT_EQ(pkt.n, 0x9abc);
}

TEST(RtpLostPacketTest, ToString) {
  RtpLostPacket pkt;
  pkt.header = {0x80, 0x60, 0x1234};
  pkt.seq_number = 0x5678;
  pkt.n = 0x9abc;
  std::string s = pkt.ToString();
  EXPECT_NE(s.find("seq_number"), std::string::npos);
}

// TODO(cattchen) fix test
// TEST(RtpSyncPacketTest, BuildAndSerialize) {
//   uint64_t ts = 44100 * 10;
//   uint64_t sr = 44100;
//   uint64_t latency = 1000;
//   RtpSyncPacket pkt = RtpSyncPacket::Build(ts, sr, latency, true);
//   uint8_t buf[20] = {0};
//   pkt.Serialize(buf);
//   EXPECT_EQ(buf[0] & 0xf0, 0x80);
// }

TEST(RtpSyncPacketTest, ToString) {
  RtpSyncPacket pkt = RtpSyncPacket::Build(44100, 44100, 1000, false);
  std::string s = pkt.ToString();
  EXPECT_NE(s.find("rtp_timestamp"), std::string::npos);
}

TEST(RtpAudioPacketTest, Serialize) {
  RtpAudioPacket pkt;
  pkt.header = {0x80, 0x60, 0x1234};
  pkt.timestamp = 0xdeadbeef;
  pkt.ssrc = 0x12345678;
  pkt.data.len_ = 8;
  for (size_t i = 0; i < 8; ++i) pkt.data.data_[i] = i;
  std::vector<uint8_t> buf;
  pkt.Serialize(buf);
  EXPECT_EQ(buf.size(), 12 + 8);
  EXPECT_EQ(buf[0], 0x80);
  EXPECT_EQ(buf[4], 0xde);
}

TEST(VolumeTest, FromPercent) {
  Volume v0 = Volume::FromPercent(0);
  EXPECT_FLOAT_EQ(v0.GetValue(), -144.0);
  Volume v50 = Volume::FromPercent(50);
  EXPECT_GT(v50.GetValue(), -30.0);
  EXPECT_LT(v50.GetValue(), 0.0);
  Volume v100 = Volume::FromPercent(100);
  EXPECT_FLOAT_EQ(v100.GetValue(), 0.0);
}
