// Copyright (c) 2025 ChenKS12138

#pragma once

#include <netinet/in.h>

#include <string>

#include "errcode.h"

namespace AirBeamCore {
namespace helper {
struct NetAddr {
  std::string ip_;
  uint32_t port_;
  std::string ToString() const;
};

class TcpClient {
 public:
  TcpClient();
  ~TcpClient();

  ErrCode Connect(const std::string& ip, int port);
  ErrCode Write(const std::string& data);
  ErrCode Read(std::string& data);
  ErrCode GetLocalNetAddr(NetAddr& addr);
  ErrCode GetRemoteNetAddr(NetAddr& addr);
  void Close();

 private:
  int sockfd_ = -1;
  struct sockaddr_in remote_addr_ {};
  struct sockaddr_in local_addr_ {};
};

class UdpServer {
 public:
  UdpServer();
  ~UdpServer();

  ErrCode Bind();
  ErrCode Write(const std::string& data, const std::string& ip, int port);
  ErrCode Read(std::string& data, std::string& ip, int& port);
  void Close();

 private:
  int sockfd_ = -1;
  struct sockaddr_in local_addr_ {};
};

}  // namespace helper
}  // namespace AirBeamCore