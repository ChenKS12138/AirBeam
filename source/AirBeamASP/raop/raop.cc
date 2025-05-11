// Copyright (c) 2025 ChenKS12138

#include "raop.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <thread>

#include "absl/strings/numbers.h"
#include "constants.h"
#include "fmt/core.h"
#include "rtp.h"
#include "rtsp.h"

uint64_t generateRandomU64() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;
  return dis(gen);
}

static std::string generateRandomNumberString(int length) {
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<int> dist(0, 9);
  std::string result;
  result.reserve(length);
  for (int i = 0; i < length; ++i) {
    result += fmt::format("{}", dist(gen));
  }
  return result;
}

static std::string generateRandomHexString(int length) {
  static const char hex_chars[] = "0123456789abcdef";
  std::string result;
  result.reserve(length);
  for (int i = 0; i < length; ++i) {
    result += hex_chars[std::rand() % 16];
  }
  return result;
}

void Raop::Start() {
  struct sockaddr_in server_addr;
  sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd_ < 0) {
    return;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(7000);
  inet_pton(AF_INET, raop_ip_addr_.c_str(), &server_addr.sin_addr);
  if (connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    close(sockfd_);
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
    ssrc_ = static_cast<uint32_t>(generateRandomU64());
    audio_peer_addr_.sin_port = htons(remote_audio_port_);
    inet_pton(AF_INET, raop_ip_addr_.c_str(), &audio_peer_addr_.sin_addr);
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
  std::string request = fmt::format(
      "SET_PARAMETER rtsp://{}/{} "
      "RTSP/1.0\r\nContent-Type: text/parameters\r\nContent-Length: "
      "{}\r\nCSeq: {}\r\nUser-Agent: iTunes/7.6.2 (Windows; "
      "N;)\r\nClient-Instance: {}\r\nSession: 1\r\n\r\n{}",
      raop_ip_addr_, sid_, body.size(), status_.seq_number, sci_, body);
  int ret = send(sockfd_, request.c_str(), request.size(), 0);
  if (ret < 0) {
    exit(-1);
    return;
  }
  char buffer[1024];
  size_t bytes_received = recv(sockfd_, buffer, sizeof(buffer), 0);
  if (bytes_received < 0) {
    exit(-1);
    return;
  }
  buffer[bytes_received] = '\0';
}

void Raop::GenerateID() {
  constexpr int kSidLen = 10;
  sid_ = generateRandomNumberString(kSidLen);
  constexpr int kSciLen = 16;
  sci_ = generateRandomHexString(kSciLen);
}

void Raop::Announce() {
  std::string request = fmt::format(
      "ANNOUNCE rtsp://{}/{} RTSP/1.0\r\n"
      "Content-Type: application/sdp\r\n"
      "Content-Length: 141\r\n"
      "CSeq: 1\r\n"
      "User-Agent: iTunes/7.6.2 (Windows; N;)\r\n"
      "Client-Instance: {}\r\n\r\n"
      "v=0\r\no=iTunes 0812982985 0 IN IP4 "
      "192.168.123.157\r\ns=iTunes\r\nc=IN IP4 {}\r\nt=0 "
      "0\r\nm=audio 0 RTP/AVP 96\r\na=rtpmap:96 L16/44100/2\r\n",
      raop_ip_addr_, sid_, sci_, raop_ip_addr_);
  int ret = send(sockfd_, request.c_str(), request.size(), 0);
  if (ret < 0) {
    exit(-1);
    return;
  }
  char buffer[1024];
  int bytes_received = recv(sockfd_, buffer, sizeof(buffer), 0);
  if (bytes_received < 0) {
    exit(-1);
    return;
  }
  buffer[bytes_received] = '\0';
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
  std::string request = fmt::format(
      "SETUP rtsp://{}/{} RTSP/1.0\r\n"
      "Transport: "
      "RTP/AVP/"
      "UDP;unicast;interleaved=0-1;mode=record;control_port={};timing_"
      "port={}\r\n"
      "CSeq: 2\r\n"
      "User-Agent: iTunes/7.6.2 (Windows; N;)\r\n"
      "Client-Instance: {}\r\n\r\n",
      raop_ip_addr_, sid_, ctrl_port_, time_port_, sci_);
  int ret = send(sockfd_, request.c_str(), request.size(), 0);
  if (ret < 0) {
    exit(-1);
    return;
  }
  char buffer[1024];
  size_t bytes_received = recv(sockfd_, buffer, sizeof(buffer), 0);
  if (bytes_received < 0) {
    exit(-1);
    return;
  }
  buffer[bytes_received] = '\0';
  auto resp = RtspMessage::Parse({buffer, bytes_received});
  auto transport = ParseKVStr(resp.headers_["Transport"], "=", ";");
  if (!absl::SimpleAtoi(transport["server_port"], &remote_audio_port_)) {
    exit(-1);
    return;
  }
  if (!absl::SimpleAtoi(transport["control_port"], &remote_ctrl_port_)) {
    exit(-1);
    return;
  }
  if (!absl::SimpleAtoi(transport["timing_port"], &remote_time_port_)) {
    exit(-1);
    return;
  }
}

void Raop::Record() {
  uint64_t start_seq = status_.seq_number + 1;
  uint64_t start_ts = NtpTime::Now().IntoTimestamp(kSampleRate44100);
  std::string request = fmt::format(
      "RECORD rtsp://{}/0812982985 RTSP/1.0\r\nRange: "
      "npt=0-\r\nRTP-Info: seq={};rtptime={}\r\nCSeq: "
      "3\r\nUser-Agent: iTunes/7.6.2 (Windows; N;)\r\nClient-Instance: "
      "8fae761ff0c7c827\r\nSession: 1\r\n\r\n",
      raop_ip_addr_, start_seq, start_ts);
  int ret = send(sockfd_, request.c_str(), request.size(), 0);
  if (ret < 0) {
    exit(-1);
    return;
  }
  char buffer[1024];
  size_t bytes_received = recv(sockfd_, buffer, sizeof(buffer), 0);
  if (bytes_received < 0) {
    exit(-1);
    return;
  }
  buffer[bytes_received] = '\0';
  auto resp = RtspMessage::Parse({buffer, bytes_received});
  resp.headers_["Audio-Latency"];
  if (!absl::SimpleAtoi(resp.headers_["Audio-Latency"], &latency_)) {
    exit(-1);
    return;
  }
}

void Raop::SyncStart() {
  ctrl_peer_addr_.sin_port = htons(remote_ctrl_port_);
  inet_pton(AF_INET, raop_ip_addr_.c_str(), &ctrl_peer_addr_.sin_addr);
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
      std::string request = fmt::format(
          "OPTIONS * RTSP/1.0\r\nCSeq: 5\r\nUser-Agent: iTunes/7.6.2 "
          "(Windows; "
          "N;)\r\nClient-Instance: {}\r\nSession: 1\r\n\r\n",
          sci_);
      int ret = send(sockfd_, request.c_str(), request.size(), 0);
      if (ret < 0) {
        exit(-1);
        return;
      }
      char buffer[1024];
      int bytes_received = recv(sockfd_, buffer, sizeof(buffer), 0);
      if (bytes_received < 0) {
        exit(-1);
        return;
      }
      buffer[bytes_received] = '\0';
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
