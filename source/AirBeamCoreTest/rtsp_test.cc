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

TEST(JoinKVStrTest, Basic) {
  std::map<std::string, std::string> data = {
      {"a", "1"}, {"b", "2"}, {"c", "3"}};
  std::string result = JoinKVStr(data, ":", ";");
  EXPECT_EQ(result, "a:1;b:2;c:3");
}

TEST(JoinKVStrTest, EmptyMap) {
  std::map<std::string, std::string> data;
  std::string result = JoinKVStr(data, ":", ";");
  EXPECT_EQ(result, "");
}

TEST(JoinKVStrTest, CustomDelimiter) {
  std::map<std::string, std::string> data = {{"k1", "val1"}, {"k2", "val2"}};
  std::string result = JoinKVStr(data, "=", "|");
  EXPECT_EQ(result, "k1=val1|k2=val2");
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
  EXPECT_EQ(msg.GetStartLine(),
            "OPTIONS rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(msg.GetHeader("CSeq"), "1");
  EXPECT_EQ(msg.GetHeader("User-Agent"), "TestAgent");
  EXPECT_EQ(msg.GetBody(), "body_content");
}

TEST(RtspMessageTest, ParseNoBody) {
  std::string content =
      "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\n"
      "CSeq: 2\r\n"
      "\r\n";
  RtspMessage msg = RtspMessage::Parse(content);
  EXPECT_EQ(msg.GetStartLine(),
            "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(msg.GetHeader("CSeq"), "2");
  EXPECT_EQ(msg.GetBody(), "");
}

TEST(RtspMessageTest, ToString) {
  RtspRespMessage msg =
      RtspMsgBuilder<RtspRespMessage>()
          .SetStatusCode(200)
          .SetStatusText("OK")
          .AddHeader("CSeq", "1")
          .AddHeader("Content-Type", "application/sdp")
          .SetBody(
              "v=0\r\n"
              "o=- 915761136 1 IN IP4 192.168.3.10\r\n"
              "s=Unnamed\r\n"
              "i=RTP/AVP session\r\n"
              "c=IN IP4 0.0.0.0\r\n"
              "t=0 0\r\n"
              "m=audio 0 RTP/AVP 96\r\n"
              "a=rtpmap:96 mpeg4-generic/44100/2\r\n"
              "a=fmtp:96 streamtype=5; profile-level-id=15; "
              "mode=AAC-hbr; sizelength=13; indexlength=3; "
              "indexdeltalength=3;\r\n"
              "a=cliprect:0,0,320,240\r\n"
              "a=framerate:29.970000\r\n"
              "a=control:rtsp://192.168.3.10:7000/raop_0/audio")
          .Build();

  std::string expected_string =
      "RTSP/1.0 200 OK\r\n"
      "CSeq: 1\r\n"
      "Content-Type: application/sdp\r\n"
      "\r\n"
      "v=0\r\n"
      "o=- 915761136 1 IN IP4 192.168.3.10\r\n"
      "s=Unnamed\r\n"
      "i=RTP/AVP session\r\n"
      "c=IN IP4 0.0.0.0\r\n"
      "t=0 0\r\n"
      "m=audio 0 RTP/AVP 96\r\n"
      "a=rtpmap:96 mpeg4-generic/44100/2\r\n"
      "a=fmtp:96 streamtype=5; profile-level-id=15; mode=AAC-hbr; "
      "sizelength=13; indexlength=3; indexdeltalength=3;\r\n"
      "a=cliprect:0,0,320,240\r\n"
      "a=framerate:29.970000\r\n"
      "a=control:rtsp://192.168.3.10:7000/raop_0/audio";

  EXPECT_EQ(msg.ToString(), expected_string);
}

TEST(RtspMessageBuilderTest, BasicBuild) {
  RtspReqMessage msg = RtspMsgBuilder<RtspReqMessage>()
                           .SetMethod("OPTIONS")
                           .SetUri("rtsp://example.com/media.mp4")
                           .AddHeader("CSeq", "1")
                           .AddHeader("User-Agent", "TestAgent")
                           .SetBody("body_content")
                           .Build();

  EXPECT_EQ(msg.GetStartLine(),
            "OPTIONS rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(msg.GetHeader("CSeq"), "1");
  EXPECT_EQ(msg.GetHeader("User-Agent"), "TestAgent");
  EXPECT_EQ(msg.GetBody(), "body_content");

  std::string content = msg.ToString();
  RtspMessage parsed_msg = RtspMessage::Parse(content);

  EXPECT_EQ(parsed_msg.GetStartLine(),
            "OPTIONS rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(parsed_msg.GetHeader("CSeq"), "1");
  EXPECT_EQ(parsed_msg.GetHeader("User-Agent"), "TestAgent");
  EXPECT_EQ(parsed_msg.GetBody(), "body_content");
}

TEST(RtspMessageBuilderTest, NoBodyBuild) {
  RtspReqMessage msg = RtspMsgBuilder<RtspReqMessage>()
                           .SetMethod("DESCRIBE")
                           .SetUri("rtsp://example.com/media.mp4")
                           .AddHeader("CSeq", "2")
                           .Build();

  EXPECT_EQ(msg.GetStartLine(),
            "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(msg.GetHeader("CSeq"), "2");
  EXPECT_EQ(msg.GetBody(), "");

  std::string content = msg.ToString();
  RtspMessage parsed_msg = RtspMessage::Parse(content);

  EXPECT_EQ(parsed_msg.GetStartLine(),
            "DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0");
  EXPECT_EQ(parsed_msg.GetHeader("CSeq"), "2");
  EXPECT_EQ(parsed_msg.GetBody(), "");
}
