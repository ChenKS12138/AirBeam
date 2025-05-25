// Copyright (c) 2025 ChenKS12138

#include "raop.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>

#include "absl/strings/numbers.h"
#include "constants.h"
#include "fmt/core.h"
#include "helper/errcode.h"
#include "helper/logger.h"
#include "helper/network.h"
#include "helper/random.h"
#include "rtp.h"
#include "rtsp.h"

namespace AirBeamCore {

namespace raop {

using namespace helper;

void Raop::Start() {
  int ret = rtsp_client_.Connect(rtsp_ip_addr_, rtsp_port_);
  if (ret != kOk) {
    ABDebugLog("rtsp_client_.Connect failed, ret=%d", ret);
    return;
  }

  GenerateID();
  Announce();
  BindCtrlAndTimePort();
  Setup();
  Record();
  SyncStart();
  KeepAlive();
  FirstSendSync();
  {
    ssrc_ =
        static_cast<uint32_t>(helper::RandomGenerator::GetInstance().GenU64());
  }
  is_started_ = true;
}

void Raop::AcceptFrame() {
  if (!is_started_) return;
  auto now = NtpTime::Now();
  uint64_t now_ts = now.IntoTimestamp(kSampleRate44100);
  if (now_ts < (status_.head_ts + kPCMChunkLength)) {
    uint64_t sleep_frames = (status_.head_ts + kPCMChunkLength) - now_ts;
    auto sleep_duration =
        std::chrono::seconds(sleep_frames / kSampleRate44100) +
        std::chrono::nanoseconds(static_cast<uint32_t>(
            (sleep_frames % kSampleRate44100) *
            static_cast<uint64_t>(UINT32_MAX) / (kSampleRate44100 - 1)));
    std::this_thread::sleep_for(sleep_duration);
  }
}

void Raop::SendChunk(const RtpAudioPacketChunk& chunk) {
  if (!is_started_) return;
  status_.seq_number += 1;
  RtpAudioPacket packet;
  packet.header.proto = 0x80;
  packet.header.type = first_pkt_ ? 0xE0 : 0x60;
  packet.header.seq = status_.seq_number;
  packet.timestamp = status_.head_ts;
  packet.ssrc = ssrc_;
  packet.data = chunk;
  first_pkt_ = false;
  std::vector<uint8_t> buffer;
  packet.Serialize(buffer);
  std::string data;
  data.resize(buffer.size());
  memcpy(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data.c_str())),
         buffer.data(), buffer.size());
  int ret = audio_server_.Write(remote_audio_addr_, data);
  if (ret != kOk) {
    ABDebugLog("audio_server_.Write failed, ret=%d", ret);
    exit(-1);
    return;
  }
  status_.head_ts += kPCMChunkLength;
}

void Raop::SetVolume(uint8_t volume_percent) {
  status_.seq_number += 1;
  Volume volume = Volume::FromPercent(volume_percent);
  std::string body = fmt::format("volume: {}\r\n", volume.GetValue());
  std::string uri = fmt::format("rtsp://{}/{}", rtsp_ip_addr_, sid_);

  auto request =
      AirBeamCore::raop::RtspMsgBuilder<AirBeamCore::raop::RtspReqMessage>()
          .SetMethod("SET_PARAMETER")
          .SetUri(uri)
          .AddHeader("Content-Type", "text/parameters")
          .AddHeader("Content-Length", std::to_string(body.size()))
          .AddHeader("CSeq", std::to_string(status_.seq_number++))
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .AddHeader("Session", "1")
          .SetBody(body)
          .Build();
  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    ABDebugLog("rtsp_client_.DoRequest failed, ret=%d", ret);
    exit(-1);
    return;
  }
}

void Raop::GenerateID() {
  constexpr int kSidLen = 10;
  sid_ = helper::RandomGenerator::GetInstance().GenNumStr(kSidLen);
  constexpr int kSciLen = 16;
  sci_ = helper::RandomGenerator::GetInstance().GenHexStr(kSciLen);
}

void Raop::Announce() {
  std::string uri = fmt::format("rtsp://{}/{}", rtsp_ip_addr_, sid_);

  std::vector<std::tuple<std::string, std::string>> sdp_map = {
      {"v", "0"},
      {"o", fmt::format("iTunes {} 0 IN IP4 {}", sid_,
                        rtsp_client_.GetLocalNetAddr().ip_)},
      {"s", "iTunes"},
      {"c", fmt::format("IN IP4 {}", rtsp_ip_addr_)},
      {"t", "0 0"},
      {"m", "audio 0 RTP/AVP 96"},
      {"a", "rtpmap:96 L16/44100/2"}};
  std::string sdp = JoinKVStrOrdered(sdp_map, "=", "\r\n") + "\r\n";

  auto request =
      AirBeamCore::raop::RtspMsgBuilder<AirBeamCore::raop::RtspReqMessage>()
          .SetMethod("ANNOUNCE")
          .SetUri(uri)
          .AddHeader("Content-Type", "application/sdp")
          .AddHeader("Content-Length", std::to_string(sdp.size()))
          .AddHeader("CSeq", std::to_string(status_.seq_number++))
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .SetBody(sdp)
          .Build();
  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    ABDebugLog("rtsp_client_.DoRequest failed, ret=%d", ret);
    exit(-1);
    return;
  }
}

void Raop::BindCtrlAndTimePort() {
  int ret = 0;
  ret = ctrl_server_.Bind();
  if (ret != kOk) {
    ABDebugLog("ctrl_server_.Bind, ret=%d", ret);
    exit(-1);
    return;
  }
  ret = time_server_.Bind();
  if (ret != kOk) {
    ABDebugLog("time_server_.Bind, ret=%d", ret);
    exit(-1);
    return;
  }
  (new std::thread([&]() {
    helper::NetAddr remote_addr;
    uint8_t buffer[32];
    std::string data;
    while (true) {
      memset(buffer, 0, sizeof(buffer));
      int ret = time_server_.Read(remote_addr, data);
      if (ret != kOk) {
        ABDebugLog("time_server_.Read failed, ret=%d", ret);
        exit(-1);
        return;
      }
      memcpy(buffer, data.data(), data.size());
      auto recv_pkt = RtpTimePacket::Deserialize(buffer, data.size());
      RtpTimePacket send_pkt;
      send_pkt.header.proto = recv_pkt.header.proto;
      send_pkt.header.type = 0x53 | 0x80;
      send_pkt.header.seq = recv_pkt.header.seq;
      send_pkt.dummy = 0;
      send_pkt.recv_time = NtpTime::Now();
      send_pkt.ref_time = recv_pkt.send_time;
      send_pkt.send_time = NtpTime::Now();
      memset(buffer, 0, 32);
      send_pkt.Serialize(buffer);
      data.resize(sizeof(buffer), '\0');
      memcpy(data.data(), buffer, sizeof(buffer));
      ret = time_server_.Write(remote_addr, data);
      if (ret != kOk) {
        ABDebugLog("time_server_.Write failed, ret=%d", ret);
        exit(-1);
        return;
      }
    }
  }))->detach();
  ret = audio_server_.Bind();
  if (ret != kOk) {
    ABDebugLog("audio_server_.Bind failed, ret=%d", ret);
    exit(-1);
  }
}

void Raop::Setup() {
  int ret = 0;

  ABDebugLog("Raop::Setup ctrl server port =%d",
             ctrl_server_.GetLocalNetAddr().port_);

  std::string uri = fmt::format("rtsp://{}/{}", rtsp_ip_addr_, sid_);
  std::vector<std::tuple<std::string, std::string>> transport_params = {
      {"interleaved", "0-1"},
      {"mode", "record"},
      {"control_port", std::to_string(ctrl_server_.GetLocalNetAddr().port_)},
      {"timing_port", std::to_string(time_server_.GetLocalNetAddr().port_)},
  };
  std::string transport =
      "RTP/AVP/UDP;unicast;" + JoinKVStrOrdered(transport_params, "=", ";");

  auto request =
      AirBeamCore::raop::RtspMsgBuilder<AirBeamCore::raop::RtspReqMessage>()
          .SetMethod("SETUP")
          .SetUri(uri)
          .AddHeader("Transport", transport)
          .AddHeader("CSeq", std::to_string(status_.seq_number++))
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .Build();
  RtspRespMessage response;
  ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    ABDebugLog("rtsp_client_.DoRequest failed, ret=%d", ret);
    exit(-1);
    return;
  }

  auto transport_map = ParseKVStr(response.GetHeader("Transport"), "=", ";");
  if (!absl::SimpleAtoi(transport_map["server_port"],
                        &remote_audio_addr_.port_)) {
    ABDebugLog("absl::SimpleAtoi(transport_map[\"server_port\"], ...) failed");
    exit(-1);
    return;
  }
  if (!absl::SimpleAtoi(transport_map["control_port"],
                        &remote_ctrl_addr_.port_)) {
    ABDebugLog("absl::SimpleAtoi(transport_map[\"control_port\"], ...) failed");
    exit(-1);
    return;
  }
  if (!absl::SimpleAtoi(transport_map["timing_port"],
                        &remote_time_addr_.port_)) {
    ABDebugLog("absl::SimpleAtoi(transport_map[\"timing_port\"], ...) failed");
    exit(-1);
    return;
  }

  remote_audio_addr_.ip_ = rtsp_client_.GetRemoteNetAddr().ip_;
  remote_ctrl_addr_.ip_ = rtsp_client_.GetRemoteNetAddr().ip_;
  remote_time_addr_.ip_ = rtsp_client_.GetRemoteNetAddr().ip_;
}

void Raop::Record() {
  uint64_t start_seq = status_.seq_number++;
  uint64_t start_ts = NtpTime::Now().IntoTimestamp(kSampleRate44100);
  std::string uri = fmt::format("rtsp://{}/{}", rtsp_ip_addr_, sid_);
  std::string range = "npt=0-";
  std::vector<std::tuple<std::string, std::string>> rtp_info_map = {
      {"seq", std::to_string(start_seq)},
      {"rtptime", std::to_string(start_ts)},
  };
  std::string rtp_info = JoinKVStrOrdered(rtp_info_map, "=", ";");
  auto request =
      AirBeamCore::raop::RtspMsgBuilder<AirBeamCore::raop::RtspReqMessage>()
          .SetMethod("RECORD")
          .SetUri(uri)
          .AddHeader("Range", range)
          .AddHeader("RTP-Info", rtp_info)
          .AddHeader("CSeq", std::to_string(start_seq))
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .AddHeader("Session", "1")
          .Build();

  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    ABDebugLog("rtsp_client_.DoRequest failed, ret=%d", ret);
    exit(-1);
    return;
  }
  response.GetHeader("Audio-Latency");
  if (!absl::SimpleAtoi(response.GetHeader("Audio-Latency"), &latency_)) {
    ABDebugLog(
        "absl::SimpleAtoi(response.GetHeader(\"Audio-Latency\"), ...) failed");
    exit(-1);
    return;
  }
}

void Raop::SyncStart() {
  (new std::thread([this]() {
    NetAddr ctrl_remote_addr;
    while (true) {
      std::string buffer;
      int ret = ctrl_server_.Read(ctrl_remote_addr, buffer);
      if (ret != kOk) {
        ABDebugLog("ctrl_server_.Read failed, ret=%d", ret);
        exit(-1);
        return;
      }
      auto recv_pkt = RtpLostPacket::Deserialize(
          reinterpret_cast<uint8_t*>(buffer.data()), buffer.size());
    }
  }))->detach();
  (new std::thread([this]() {
    while (true) {
      auto rsp = RtpSyncPacket::Build(status_.head_ts, kSampleRate44100,
                                      latency_, false);
      uint8_t buffer[sizeof(RtpSyncPacket)];
      rsp.Serialize(buffer);
      std::string data;
      data.resize(sizeof(RtpSyncPacket), '\0');
      memcpy(data.data(), buffer, sizeof(RtpSyncPacket));
      int ret = ctrl_server_.Write(remote_ctrl_addr_, data);
      if (ret != kOk) {
        ABDebugLog("ctrl_server_.Write failed, ret=%d", ret);
        exit(-1);
        return;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }))->detach();
}

void Raop::KeepAlive() {
  (new std::thread([this]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      auto request =
          AirBeamCore::raop::RtspMsgBuilder<AirBeamCore::raop::RtspReqMessage>()
              .SetMethod("OPTIONS")
              .SetUri("*")
              .AddHeader("CSeq", std::to_string(status_.seq_number++))
              .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
              .AddHeader("Client-Instance", sci_)
              .AddHeader("Session", "1")
              .Build();
      RtspRespMessage response;
      int ret = rtsp_client_.DoRequest(request, response);
      if (ret != kOk) {
        ABDebugLog("rtsp_client_.DoRequest failed, ret=%d", ret);
        exit(-1);
        return;
      }
    }
  }))->detach();
}

void Raop::FirstSendSync() {
  uint64_t now_ts = NtpTime::Now().IntoTimestamp(kSampleRate44100);
  status_.head_ts = now_ts;
  status_.first_ts = status_.head_ts;
  auto pkt =
      RtpSyncPacket::Build(status_.head_ts, kSampleRate44100, latency_, true);
  uint8_t buffer[sizeof(RtpSyncPacket)];
  pkt.Serialize(buffer);
  std::string data;
  data.resize(sizeof(RtpSyncPacket), '\0');
  memcpy(data.data(), buffer, sizeof(RtpSyncPacket));
  int ret = ctrl_server_.Write(remote_ctrl_addr_, data);
  ABDebugLog("ctrl_server_.Write,len=%lu", data.size());
  if (ret != kOk) {
    ABDebugLog("ctrl_server_.Write failed, ret=%d", ret);
    exit(-1);
    return;
  }
}
}  // namespace raop

}  // namespace AirBeamCore
