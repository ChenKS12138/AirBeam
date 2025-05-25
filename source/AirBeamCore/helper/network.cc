// Copyright (c) 2025 ChenKS12138

#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "errcode.h"

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
  memset(&remote_addr_, 0, sizeof(remote_addr_));
  remote_addr_.sin_family = AF_INET;
  remote_addr_.sin_port = htons(port);
  if (inet_pton(AF_INET, ip.c_str(), &remote_addr_.sin_addr) <= 0)
    return kErrTcpAddrParse;
  if (connect(sockfd_, (sockaddr*)&remote_addr_, sizeof(remote_addr_)) < 0)
    return kErrTcpConnect;
  socklen_t len = sizeof(local_addr_);
  getsockname(sockfd_, (sockaddr*)&local_addr_, &len);
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

ErrCode TCPClient::GetLocalNetAddr(NetAddr& addr) {
  if (sockfd_ < 0) return kErrTcpSocketCreate;
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &local_addr_.sin_addr, ip, sizeof(ip));
  addr.ip_ = ip;
  addr.port_ = ntohs(local_addr_.sin_port);
  return kOk;
}

ErrCode TCPClient::GetRemoteNetAddr(NetAddr& addr) {
  if (sockfd_ < 0) return kErrTcpSocketCreate;
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &remote_addr_.sin_addr, ip, sizeof(ip));
  addr.ip_ = ip;
  addr.port_ = ntohs(remote_addr_.sin_port);
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
  memset(&local_addr_, 0, sizeof(local_addr_));
  local_addr_.sin_family = AF_INET;
  local_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr_.sin_port = htons(0);  // random
  if (bind(sockfd_, (sockaddr*)&local_addr_, sizeof(local_addr_)) < 0)
    return kErrUdpBind;
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
ErrCode UDPServer::GetLocalNetAddr(NetAddr& addr) {
  if (sockfd_ < 0) return kErrUdpSocketCreate;
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &local_addr_.sin_addr, ip, sizeof(ip));
  addr.ip_ = ip;
  addr.port_ = ntohs(local_addr_.sin_port);
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
