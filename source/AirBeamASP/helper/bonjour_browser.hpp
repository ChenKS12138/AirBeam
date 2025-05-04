#pragma once

#include <arpa/inet.h>
#include <dns_sd.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

class BonjourBrowser {
 protected:
  struct ContextForAddr {
    BonjourBrowser* browser;
    std::string serviceName;
    std::string fullName;
    uint16_t port;
  };

 public:
  using ServiceFoundCallback =
      std::function<void(const std::string& name, const std::string& fullname,
                         const std::string& ip, uint16_t port)>;

  BonjourBrowser() : browseRef_(nullptr), running_(false) {}

  ~BonjourBrowser() { stop(); }

  bool startBrowse(const std::string& serviceType,
                   ServiceFoundCallback callback) {
    if (running_) return false;

    callback_ = callback;

    DNSServiceErrorType err =
        DNSServiceBrowse(&browseRef_, 0, 0, serviceType.c_str(), nullptr,
                         &BonjourBrowser::BrowseCallback, this);

    if (err != kDNSServiceErr_NoError) {
      std::cerr << "DNSServiceBrowse failed: " << err << std::endl;
      return false;
    }

    running_ = true;
    browseThread_ = std::thread(&BonjourBrowser::browseLoop, this);
    return true;
  }

  void stop() {
    if (!running_) return;

    running_ = false;

    if (browseRef_) {
      DNSServiceRefDeallocate(browseRef_);
      browseRef_ = nullptr;
    }

    if (browseThread_.joinable()) {
      browseThread_.join();
    }
  }

 private:
  DNSServiceRef browseRef_;
  std::thread browseThread_;
  std::atomic<bool> running_;
  ServiceFoundCallback callback_;

  void browseLoop() {
    int fd = DNSServiceRefSockFD(browseRef_);
    if (fd == -1) return;

    while (running_) {
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(fd, &readfds);

      struct timeval tv;
      tv.tv_sec = 5;
      tv.tv_usec = 0;

      int result = select(fd + 1, &readfds, nullptr, nullptr, &tv);
      if (result > 0 && FD_ISSET(fd, &readfds)) {
        DNSServiceProcessResult(browseRef_);
      }
    }
  }

  static void DNSSD_API BrowseCallback(
      DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
      DNSServiceErrorType errorCode, const char* serviceName,
      const char* regtype, const char* replyDomain, void* context) {
    if (errorCode != kDNSServiceErr_NoError) return;

    BonjourBrowser* self = static_cast<BonjourBrowser*>(context);

    if (flags & kDNSServiceFlagsAdd) {
      DNSServiceRef resolveRef;
      DNSServiceErrorType err = DNSServiceResolve(
          &resolveRef, 0, interfaceIndex, serviceName, regtype, replyDomain,
          &BonjourBrowser::ResolveCallback, self);

      if (err == kDNSServiceErr_NoError) {
        int fd = DNSServiceRefSockFD(resolveRef);
        if (fd != -1) {
          fd_set readfds;
          FD_ZERO(&readfds);
          FD_SET(fd, &readfds);
          struct timeval tv;
          tv.tv_sec = 5;
          tv.tv_usec = 0;

          int result = select(fd + 1, &readfds, nullptr, nullptr, &tv);
          if (result > 0 && FD_ISSET(fd, &readfds)) {
            DNSServiceProcessResult(resolveRef);
          }
        }
        DNSServiceRefDeallocate(resolveRef);
      }
    }
  }

  static void DNSSD_API ResolveCallback(
      DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
      DNSServiceErrorType errorCode, const char* fullname,
      const char* hosttarget, uint16_t port, uint16_t txtLen,
      const unsigned char* txtRecord, void* context) {
    if (errorCode != kDNSServiceErr_NoError) return;

    BonjourBrowser* self = static_cast<BonjourBrowser*>(context);
    if (!self) return;

    port = ntohs(port);

    // 记录 fullname
    ContextForAddr* ctx = new ContextForAddr{self, "", fullname, port};

    DNSServiceRef addrRef;
    DNSServiceErrorType err = DNSServiceGetAddrInfo(
        &addrRef, 0, interfaceIndex, kDNSServiceProtocol_IPv4, hosttarget,
        &BonjourBrowser::GetAddrInfoCallback, ctx);

    if (err == kDNSServiceErr_NoError) {
      int fd = DNSServiceRefSockFD(addrRef);
      if (fd != -1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int result = select(fd + 1, &readfds, nullptr, nullptr, &tv);
        if (result > 0 && FD_ISSET(fd, &readfds)) {
          DNSServiceProcessResult(addrRef);
        }
      }
      DNSServiceRefDeallocate(addrRef);
    }
  }

  static void DNSSD_API GetAddrInfoCallback(
      DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
      DNSServiceErrorType errorCode, const char* hostname,
      const struct sockaddr* address, uint32_t ttl, void* context) {
    if (errorCode != kDNSServiceErr_NoError) return;
    if (!context) return;

    ContextForAddr* ctx = static_cast<ContextForAddr*>(context);
    BonjourBrowser* self = ctx->browser;

    char ipStr[INET6_ADDRSTRLEN] = {0};
    if (address->sa_family == AF_INET) {
      inet_ntop(AF_INET, &((struct sockaddr_in*)address)->sin_addr, ipStr,
                sizeof(ipStr));
    } else if (address->sa_family == AF_INET6) {
      inet_ntop(AF_INET6, &((struct sockaddr_in6*)address)->sin6_addr, ipStr,
                sizeof(ipStr));
    }

    if (self && self->callback_) {
      std::string shortName = extractShortName(ctx->fullName);
      self->callback_(shortName, ctx->fullName, ipStr, ctx->port);
    }

    delete ctx;
  }

  static std::string extractShortName(const std::string& fullname) {
    // 从 fullname 提取出服务名（去掉 ._raop._tcp.local.）
    size_t pos = fullname.find("._");
    if (pos != std::string::npos) {
      return fullname.substr(0, pos);
    }
    return fullname;
  }
};