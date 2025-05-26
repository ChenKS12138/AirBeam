// Copyright (c) 2025 ChenKS12138

#pragma once

#include <arpa/inet.h>
#include <dns_sd.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

namespace AirBeamCore {
namespace macos {
class BonjourBrowser {
 public:
  enum class EventType {
    kServiceOnline = 1,
    kServiceOffline = 2,
  };
  struct ServiceInfo {
    std::string name;
    std::string fullname;
    std::string ip;
    uint16_t port;
    bool operator==(const ServiceInfo& other) const;
  };

 protected:
  struct ContextForAddr {
    BonjourBrowser* browser;
    std::string serviceName;
    std::string fullName;
    uint16_t port;
  };

 public:
  using ServiceFoundCallback =
      std::function<void(EventType, const ServiceInfo&)>;

  BonjourBrowser();
  ~BonjourBrowser();

  bool startBrowse(const std::string& serviceType,
                   ServiceFoundCallback callback);
  void stop();

 private:
  DNSServiceRef browseRef_;
  std::thread browseThread_;
  std::atomic<bool> running_;
  ServiceFoundCallback callback_;

  void browseLoop();
  static void DNSSD_API BrowseCallback(
      DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
      DNSServiceErrorType errorCode, const char* serviceName,
      const char* regtype, const char* replyDomain, void* context);
  static void DNSSD_API ResolveCallback(
      DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
      DNSServiceErrorType errorCode, const char* fullname,
      const char* hosttarget, uint16_t port, uint16_t txtLen,
      const unsigned char* txtRecord, void* context);
  static void DNSSD_API GetAddrInfoCallback(
      DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
      DNSServiceErrorType errorCode, const char* hostname,
      const struct sockaddr* address, uint32_t ttl, void* context);
  static std::string extractShortName(const std::string& fullname);
};
}  // namespace macos
}  // namespace AirBeamCore