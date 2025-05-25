// Copyright (c) 2025 ChenKS12138

#pragma once

#include <string>

namespace AirBeamCore {
namespace helper {
class TcpClient {
 public:
  TcpClient();
  int Connect(const std::string& ip, int port);
  int Write(const std::string& data);
  int Read(std::string& data);
  void Close();
  ~TcpClient();
};

class UdpServer {
  //
};

}  // namespace helper
}  // namespace AirBeamCore