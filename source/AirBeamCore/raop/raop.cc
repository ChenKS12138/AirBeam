// Copyright (c) 2025 ChenKS12138

#include "raop.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <thread>

#include "absl/strings/numbers.h"
#include "constants.h"
#include "fmt/core.h"
#include "helper/errcode.h"
#include "helper/random.h"
#include "rtp.h"
#include "rtsp.h"

namespace AirBeamCore {

namespace raop {

using namespace helper;

void Raop::Start() {
  int ret = rtsp_client_.Connect(rtsp_ip_addr_, rtsp_port_);
  if (ret != kOk) {
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
    audio_peer_addr_.sin_port = htons(remote_audio_port_);
    inet_pton(AF_INET, rtsp_ip_addr_.c_str(), &audio_peer_addr_.sin_addr);
    audio_peer_addr_.sin_family = AF_INET;
    audio_peer_addr_len_ = sizeof(audio_peer_addr_);
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
  int send_ret =
      sendto(audio_sockfd_, buffer.data(), buffer.size(), 0,
             (struct sockaddr*)&audio_peer_addr_, ctrl_peer_addr_len_);
  if (send_ret < 0) {
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
          .AddHeader("CSeq", std::to_string(status_.seq_number))
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .AddHeader("Session", "1")
          .SetBody(body)
          .Build();
  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
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
  // TODO (cattchen) remove hardcode ip
  std::vector<std::tuple<std::string, std::string>> sdp_map = {
      {"v", "0"},
      {"o", fmt::format("iTunes {} 0 IN IP4 192.168.123.157", sid_)},
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
          .AddHeader("CSeq", "1")
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .SetBody(sdp)
          .Build();
  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    exit(-1);
    return;
  }
}

void Raop::BindCtrlAndTimePort() {
  ctrl_port_ = 0;
  time_port_ = 0;
  ctrl_sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (ctrl_sockfd_ < 0) {
    exit(-1);
  }
  struct sockaddr_in ctrl_addr;
  ctrl_addr.sin_family = AF_INET;
  ctrl_addr.sin_addr.s_addr = INADDR_ANY;
  ctrl_addr.sin_port = 0;
  if (::bind(ctrl_sockfd_, (struct sockaddr*)&ctrl_addr, sizeof(ctrl_addr)) <
      0) {
    close(ctrl_sockfd_);
    exit(-1);
  }
  socklen_t addr_len = sizeof(ctrl_addr);
  if (getsockname(ctrl_sockfd_, (struct sockaddr*)&ctrl_addr, &addr_len) < 0) {
    close(ctrl_sockfd_);
    exit(-1);
  }
  ctrl_port_ = ntohs(ctrl_addr.sin_port);
  time_sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (time_sockfd_ < 0) {
    exit(-1);
  }
  struct sockaddr_in time_addr;
  time_addr.sin_family = AF_INET;
  time_addr.sin_addr.s_addr = INADDR_ANY;
  time_addr.sin_port = 0;
  if (::bind(time_sockfd_, (struct sockaddr*)&time_addr, sizeof(time_addr)) <
      0) {
    close(time_sockfd_);
    exit(-1);
  }
  addr_len = sizeof(time_addr);
  if (getsockname(time_sockfd_, (struct sockaddr*)&time_addr, &addr_len) < 0) {
    close(time_sockfd_);
    exit(-1);
  }
  time_port_ = ntohs(time_addr.sin_port);
  (new std::thread([time_sockfd_ = time_sockfd_]() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    while (true) {
      uint8_t buffer[32];
      int bytes_received = recvfrom(time_sockfd_, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&clientAddr, &clientLen);
      if (bytes_received < 0) {
        exit(-1);
        return;
      }
      auto recv_pkt = RtpTimePacket::Deserialize(buffer, bytes_received);
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
      int send_ret = sendto(time_sockfd_, buffer, 32, 0,
                            (struct sockaddr*)&clientAddr, clientLen);
      if (send_ret < 0) {
        exit(-1);
        return;
      }
    }
  }))->detach();
  audio_sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (audio_sockfd_ < 0) {
    exit(-1);
  }
  struct sockaddr_in audio_addr;
  audio_addr.sin_family = AF_INET;
  audio_addr.sin_addr.s_addr = INADDR_ANY;
  audio_addr.sin_port = 0;
  if (::bind(audio_sockfd_, (struct sockaddr*)&audio_addr, sizeof(audio_addr)) <
      0) {
    close(audio_sockfd_);
    exit(-1);
  }
  addr_len = sizeof(audio_addr);
  if (getsockname(audio_sockfd_, (struct sockaddr*)&audio_addr, &addr_len) <
      0) {
    close(audio_sockfd_);
    exit(-1);
  }
  audio_port_ = ntohs(audio_addr.sin_port);
}

void Raop::Setup() {
  std::string uri = fmt::format("rtsp://{}/{}", rtsp_ip_addr_, sid_);
  std::vector<std::tuple<std::string, std::string>> transport_params = {
      {"interleaved", "0-1"},
      {"mode", "record"},
      {"control_port", std::to_string(ctrl_port_)},
      {"timing_port", std::to_string(time_port_)},
  };
  std::string transport =
      "RTP/AVP/UDP;unicast;" + JoinKVStrOrdered(transport_params, "=", ";");

  auto request =
      AirBeamCore::raop::RtspMsgBuilder<AirBeamCore::raop::RtspReqMessage>()
          .SetMethod("SETUP")
          .SetUri(uri)
          .AddHeader("Transport", transport)
          .AddHeader("CSeq", "2")
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          .AddHeader("Client-Instance", sci_)
          .Build();
  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    exit(-1);
    return;
  }

  auto transport_map = ParseKVStr(response.GetHeader("Transport"), "=", ";");
  if (!absl::SimpleAtoi(transport_map["server_port"], &remote_audio_port_)) {
    exit(-1);
    return;
  }
  if (!absl::SimpleAtoi(transport_map["control_port"], &remote_ctrl_port_)) {
    exit(-1);
    return;
  }
  if (!absl::SimpleAtoi(transport_map["timing_port"], &remote_time_port_)) {
    exit(-1);
    return;
  }
}

void Raop::Record() {
  uint64_t start_seq = status_.seq_number + 1;
  uint64_t start_ts = NtpTime::Now().IntoTimestamp(kSampleRate44100);
  // TODO(catchen) remove hard code sid
  std::string uri = fmt::format("rtsp://{}/0812982985", rtsp_ip_addr_);
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
          // TODO(cattchen) remove hard code cseq
          .AddHeader("CSeq", "3")
          .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
          // TODO(cattchen) remove hard code ci
          .AddHeader("Client-Instance", "8fae761ff0c7c827")
          .AddHeader("Session", "1")
          .Build();

  RtspRespMessage response;
  int ret = rtsp_client_.DoRequest(request, response);
  if (ret != kOk) {
    exit(-1);
    return;
  }
  response.GetHeader("Audio-Latency");
  if (!absl::SimpleAtoi(response.GetHeader("Audio-Latency"), &latency_)) {
    exit(-1);
    return;
  }
}

void Raop::SyncStart() {
  ctrl_peer_addr_.sin_port = htons(remote_ctrl_port_);
  inet_pton(AF_INET, rtsp_ip_addr_.c_str(), &ctrl_peer_addr_.sin_addr);
  ctrl_peer_addr_.sin_family = AF_INET;
  ctrl_peer_addr_len_ = sizeof(ctrl_peer_addr_);
  (new std::thread([this]() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    while (true) {
      uint8_t buffer[1024];
      int bytes_received = recvfrom(ctrl_sockfd_, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&clientAddr, &clientLen);
      if (bytes_received < 0) {
        exit(-1);
        return;
      }
      auto recv_pkt = RtpLostPacket::Deserialize(buffer, bytes_received);
    }
  }))->detach();
  (new std::thread([this]() {
    while (true) {
      auto rsp = RtpSyncPacket::Build(status_.head_ts, kSampleRate44100,
                                      latency_, false);
      uint8_t buffer[sizeof(RtpSyncPacket)];
      rsp.Serialize(buffer);
      int send_ret =
          sendto(ctrl_sockfd_, buffer, sizeof(buffer), 0,
                 (struct sockaddr*)&ctrl_peer_addr_, ctrl_peer_addr_len_);
      if (send_ret < 0) {
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
              // TODO(cattchen) remove hard code cseq
              .AddHeader("CSeq", "5")
              .AddHeader("User-Agent", "iTunes/7.6.2 (Windows; N;)")
              .AddHeader("Client-Instance", sci_)
              .AddHeader("Session", "1")
              .Build();
      RtspRespMessage response;
      int ret = rtsp_client_.DoRequest(request, response);
      if (ret != kOk) {
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
  int send_ret =
      sendto(ctrl_sockfd_, buffer, sizeof(buffer), 0,
             (struct sockaddr*)&ctrl_peer_addr_, ctrl_peer_addr_len_);
  if (send_ret < 0) {
    exit(-1);
    return;
  }
}
}  // namespace raop

}  // namespace AirBeamCore
