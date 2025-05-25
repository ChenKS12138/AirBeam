// Copyright (c) 2025 ChenKS12138

#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <string>

#include "helper/network.h"
#include "helper/random.h"
#include "raop/rtp.h"
#include "raop/rtsp_client.h"

namespace AirBeamCore {
namespace raop {
struct RaopStatus {
 public:
  std::atomic<uint16_t> seq_number =
      std::atomic<uint16_t>(helper::RandomGenerator::GetInstance().GenU64());
  uint64_t head_ts = 0;
  uint64_t first_ts = 0;
};

class Raop {
 public:
  Raop(const std::string rtsp_ip_addr)
      : rtsp_ip_addr_(rtsp_ip_addr), rtsp_port_(7000) {}

 private:
  raop::RTSPClient rtsp_client_;

  helper::UDPServer ctrl_server_;
  helper::UDPServer time_server_;
  helper::UDPServer audio_server_;

  helper::NetAddr remote_audio_addr_;
  helper::NetAddr remote_time_addr_;
  helper::NetAddr remote_ctrl_addr_;

  std::string sid_;
  std::string sci_;

  int time_port_;

  int time_sockfd_;  // UDP server

  int remote_time_port_;

  uint64_t latency_ = 0;

  bool first_pkt_ = true;

  uint32_t ssrc_ = 0;

  RaopStatus status_;

  bool is_started_ = false;

  const std::string rtsp_ip_addr_;
  const uint32_t rtsp_port_;

 public:
  void Start();
  void AcceptFrame();
  void SendChunk(const RtpAudioPacketChunk& chunk);
  void SetVolume(uint8_t volume);

 private:
  void GenerateID();
  void Announce();
  void BindCtrlAndTimePort();
  void Setup();
  void Record();
  void SyncStart();
  void KeepAlive();
  void FirstSendSync();
};
}  // namespace raop
}  // namespace AirBeamCore
