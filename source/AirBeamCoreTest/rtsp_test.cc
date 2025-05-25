#include "raop/rtsp.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace AirBeamCore::raop;

TEST(ParseKVStrTest, Basic) {
  std::string content = "a:1;b:2;c:3";
  auto result = ParseKVStr(content, ":", ";");
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result["a"], "1");
  EXPECT_EQ(result["b"], "2");
  EXPECT_EQ(result["c"], "3");
}

TEST(ParseKVStrTest, EmptyContent) {
  std::string content = "";
  auto result = ParseKVStr(content, ":", ";");
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result.begin()->first, "");
  EXPECT_EQ(result.begin()->second, "");
}

TEST(ParseKVStrTest, CustomDelimiter) {
  std::string content = "k1=val1|k2=val2";
  auto result = ParseKVStr(content, "=", "|");
  EXPECT_EQ(result["k1"], "val1");
  EXPECT_EQ(result["k2"], "val2");
}

// RtspMessage 测试
TEST(RtspMessageTest, ParseBasic) {
  std::string content =
      "OPTIONS rtsp://example.com/media.mp4 RTSP/1.0\r\n"
      "CSeq: 1\r\n"
      "User-Agent: TestAgent\r\n"
      "\r\n"
      "body_content";
  RtspMessage msg = RtspMessage::Parse(content);
  EXPECT_EQ(msg.start_line_, "OPTIONS rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(msg.headers_["CSeq"], "1");
  EXPECT_EQ(msg.headers_["User-Agent"], "TestAgent");
  EXPECT_EQ(msg.body_, "body_content");
}

TEST(RtspMessageTest, ParseNoBody) {
  std::string content =
      "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\n"
      "CSeq: 2\r\n"
      "\r\n";
  RtspMessage msg = RtspMessage::Parse(content);
  EXPECT_EQ(msg.start_line_, "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(msg.headers_["CSeq"], "2");
  EXPECT_EQ(msg.body_, "");
}

TEST(RtspMessageTest, ToString) {
  RtspMessage msg;
  msg.start_line_ = "ANNOUNCE rtsp://example.com/media.mp4 RTSP/1.0";
  msg.headers_ = {{"CSeq", "3"}, {"Session", "12345"}};
  msg.body_ = "test_body";
  std::string str = msg.ToString();
  EXPECT_NE(str.find("start_line:"), std::string::npos);
  EXPECT_NE(str.find("ANNOUNCE rtsp://example.com/media.mp4 RTSP/1.0"),
            std::string::npos);
  EXPECT_NE(str.find("CSeq: 3"), std::string::npos);
  EXPECT_NE(str.find("Session: 12345"), std::string::npos);
  EXPECT_NE(str.find("body:"), std::string::npos);
  EXPECT_NE(str.find("test_body"), std::string::npos);
}
