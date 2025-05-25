// Copyright (c) 2025 ChenKS12138

#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

#include "helper/random.h"
#include "raop/rtp.h"

namespace AirBeamCore {
namespace raop {
struct RaopStatus {
 public:
  uint16_t seq_number = helper::RandomGenerator::GetInstance().GenU64();
  uint64_t head_ts = 0;
  uint64_t first_ts = 0;
};

class Raop {
 public:
  Raop(const std::string raop_ip_addr) : raop_ip_addr_(raop_ip_addr) {}

 private:
  int sockfd_;
  std::string sid_;
  std::string sci_;

  int ctrl_port_;
  int time_port_;
  int audio_port_;

  int ctrl_sockfd_;
  int time_sockfd_;
  int audio_sockfd_;

  struct sockaddr_in ctrl_peer_addr_;
  socklen_t ctrl_peer_addr_len_;

  struct sockaddr_in audio_peer_addr_;
  socklen_t audio_peer_addr_len_;

  int remote_audio_port_;
  int remote_time_port_;
  int remote_ctrl_port_;

  uint64_t latency_ = 0;

  bool first_pkt_ = true;

  uint32_t ssrc_ = 0;

  RaopStatus status_;

  bool is_started_ = false;

  const std::string raop_ip_addr_;

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
