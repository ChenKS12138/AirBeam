// Copyright (c) 2025 ChenKS12138

#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "errcode.h"
#include "helper/logger.h"

namespace AirBeamCore {
namespace helper {

std::string NetAddr::ToString() const {
  return ip_ + ":" + std::to_string(port_);
}

TCPClient::TCPClient() = default;
TCPClient::~TCPClient() { Close(); }

ErrCode TCPClient::Connect(const std::string& ip, int port) {
  if (sockfd_ != -1) Close();
  sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd_ < 0) return kErrTcpSocketCreate;

  struct sockaddr_in remote_addr {};
  struct sockaddr_in local_addr {};

  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip.c_str(), &remote_addr.sin_addr) <= 0)
    return kErrTcpAddrParse;
  if (connect(sockfd_, (sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
    return kErrTcpConnect;
  socklen_t len = sizeof(local_addr);
  getsockname(sockfd_, (sockaddr*)&local_addr, &len);

  char ipaddr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &local_addr.sin_addr, ipaddr, sizeof(ipaddr));

  remote_addr_.ip_ = ip;
  remote_addr_.port_ = ntohs(remote_addr.sin_port);

  local_addr_.ip_ = ipaddr;
  local_addr_.port_ = ntohs(local_addr.sin_port);
  return kOk;
}

ErrCode TCPClient::Write(const std::string& data) {
  if (sockfd_ < 0) return kErrTcpSocketCreate;
  ssize_t sent = send(sockfd_, data.data(), data.size(), 0);
  return sent < 0 ? kErrTcpSend : kOk;
}

ErrCode TCPClient::Read(std::string& data) {
  if (sockfd_ < 0) return kErrTcpSocketCreate;
  char buf[4096];
  ssize_t n = recv(sockfd_, buf, sizeof(buf), 0);
  if (n <= 0) return kErrTcpRecv;
  data.assign(buf, n);
  return kOk;
}

void TCPClient::Close() {
  if (sockfd_ != -1) {
    close(sockfd_);
    sockfd_ = -1;
  }
}

UDPServer::UDPServer() = default;
UDPServer::~UDPServer() { Close(); }

ErrCode UDPServer::Bind() {
  if (sockfd_ != -1) Close();
  sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd_ < 0) return kErrUdpSocketCreate;
  struct sockaddr_in local_addr {};
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = htons(0);  // random
  if (bind(sockfd_, (sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    return kErrUdpBind;

  socklen_t addr_len = sizeof(local_addr);
  if (getsockname(sockfd_, (sockaddr*)&local_addr, &addr_len) < 0)
    return kErrGetsockName;

  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &local_addr.sin_addr, ip, sizeof(ip));

  local_addr_.ip_ = ip;
  local_addr_.port_ = ntohs(local_addr.sin_port);

  return kOk;
}

ErrCode UDPServer::Write(const NetAddr& remote_addr, const std::string& data) {
  if (sockfd_ < 0) return kErrUdpSocketCreate;
  sockaddr_in dest{};
  dest.sin_family = AF_INET;
  dest.sin_port = htons(remote_addr.port_);
  if (inet_pton(AF_INET, remote_addr.ip_.c_str(), &dest.sin_addr) <= 0)
    return kErrUdpAddrParse;
  ssize_t sent = sendto(sockfd_, data.data(), data.size(), 0, (sockaddr*)&dest,
                        sizeof(dest));
  return sent < 0 ? kErrUdpSend : kOk;
}

ErrCode UDPServer::Read(NetAddr& remote_addr, std::string& data) {
  if (sockfd_ < 0) return kErrUdpSocketCreate;
  char buf[4096];
  sockaddr_in src{};
  socklen_t len = sizeof(src);
  ssize_t n = recvfrom(sockfd_, buf, sizeof(buf), 0, (sockaddr*)&src, &len);
  if (n <= 0) return kErrUdpRecv;
  data.assign(buf, n);
  char ipbuf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &src.sin_addr, ipbuf, sizeof(ipbuf));
  remote_addr.ip_ = ipbuf;
  remote_addr.port_ = ntohs(src.sin_port);
  return kOk;
}

void UDPServer::Close() {
  if (sockfd_ != -1) {
    close(sockfd_);
    sockfd_ = -1;
  }
}

}  // namespace helper
}  // namespace AirBeamCore
