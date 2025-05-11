// Copyright (c) 2025 ChenKS12138

#include "aspl/Driver.hpp"

#include <CoreAudio/AudioServerPlugIn.h>
#include <CoreAudio/CoreAudio.h>
#include <arpa/inet.h>
#include <fmt/core.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include "aspl/ControlRequestHandler.hpp"
#include "aspl/Device.hpp"
#include "aspl/IORequestHandler.hpp"
#include "aspl/Plugin.hpp"
#include "helper/bonjour_browser.hpp"
#include "helper/logger.hpp"
#include "helper/volume_observer.hpp"
#include "raop/codec.h"
#include "raop/fifo.h"
#include "raop/raop.h"

namespace {
constexpr UInt32 SampleRate = 44100;
constexpr UInt32 ChannelCount = 2;

const size_t kFiFOCapacity = 30 * 1024 * 1024;  // 30MB buffer

class RaopHandler : public aspl::ControlRequestHandler,
                    public aspl::IORequestHandler {
 public:
  explicit RaopHandler(aspl::Device& device, const std::string& ip)
      : raop_(std::make_shared<Raop>(ip)),
        fifo_(kFiFOCapacity),
        device_(device) {
    auto volume_control =
        device_.GetVolumeControlByIndex(kAudioObjectPropertyScopeOutput, 0);
    volume_control->SetScalarValue(0.5);
  }

  OSStatus OnStartIO() override {
    Prepare();

    return kAudioHardwareNoError;
  }

  void OnStopIO() override {}

  void OnWriteMixedOutput(const std::shared_ptr<aspl::Stream>& stream,
                          Float64 zeroTimestamp, Float64 timestamp,
                          const void* buff, UInt32 buffBytesSize) override {
    fifo_.Write(reinterpret_cast<const uint8_t*>(buff), buffBytesSize);
  }

 public:
  std::unique_ptr<VolumeObserver> volume_observer_ = nullptr;

 private:
  std::shared_ptr<Raop> raop_;
  ConcurrentByteFIFO fifo_;
  aspl::Device& device_;

  std::unique_ptr<std::thread> consumer_thread_;

  void Prepare() {
    if (consumer_thread_) {
      return;
    }

    raop_->Start();
    consumer_thread_ = std::make_unique<std::thread>([&]() {
      while (true) {
        RtpAudioPacketChunk chunk, encoded;
        size_t read_cnt = fifo_.Read(chunk.data_, sizeof(chunk.data_));
        chunk.len_ = read_cnt;
        if (read_cnt == 0) {
          continue;
        }

        PCMCodec::Encode(chunk, encoded);
        encoded.len_ = chunk.len_;

        raop_->AcceptFrame();
        raop_->SendChunk(encoded);
      }
    });
    consumer_thread_->detach();

    (new std::thread([=]() {
      auto device_uid = device_.GetDeviceUID();

      AudioObjectID output_device_id = 0;
      while (true) {
        OSStatus status =
            VolumeObserver::FindAudioDeviceByUID(device_uid, output_device_id);

        if (status == noErr) {
          break;
        }
        if (status == kAudioHardwareUnknownPropertyError) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
        return;
      }

      raop_->SetVolume(50);
      volume_observer_ = std::make_unique<VolumeObserver>(
          output_device_id, [=](Float32 volume) {
            uint8_t volume_percent = volume * 100;
            raop_->SetVolume(volume_percent);
          });
    }))->detach();
  }
};

class DriverHelper {
 protected:
  struct DeviceInfo {
    std::shared_ptr<aspl::Device> device_;
    BonjourBrowser::ServiceInfo service_info_;
  };

 public:
  DriverHelper()
      : context_(std::make_shared<aspl::Context>()),
        plugin_(std::make_shared<aspl::Plugin>(context_)),
        driver_(std::make_shared<aspl::Driver>(context_, plugin_)),
        devices_mapping_() {
    static const std::string kServiceType = "_raop._tcp";
    using namespace std::placeholders;
    ABDebugLog("DriverHelper startBrowse %s", kServiceType.c_str());
    browser_.startBrowse(
        kServiceType,
        std::bind(&DriverHelper::HandleBrowseCallback, this, _1, _2));
  }

  std::shared_ptr<aspl::Driver> GetDriver() { return driver_; }

 private:
  void HandleBrowseCallback(BonjourBrowser::EventType event_type,
                            const BonjourBrowser::ServiceInfo& service_info) {
    using namespace std::chrono;

    std::lock_guard<std::mutex> guard(device_mapping_mutex_);

    if (event_type == BonjourBrowser::EventType::kServiceOnline) {
      ABDebugLog("service online %s %s %s %lu", service_info.fullname.c_str(),
                 service_info.name.c_str(), service_info.ip.c_str(),
                 service_info.port);
      auto it = devices_mapping_.find(service_info.fullname);
      if (it != devices_mapping_.end()) {
        if (it->second.service_info_ == service_info) {
          ABDebugLog("device already exists %s", service_info.fullname.c_str());
          return;
        }
        ABDebugLog("device exists service changed %s",
                   service_info.fullname.c_str());
        RemoveDevice(it->second.service_info_);
      }

      AddDevice(service_info);
      ABDebugLog("device %s added", service_info.fullname.c_str());
      return;
    }

    if (event_type == BonjourBrowser::EventType::kServiceOffline) {
      ABDebugLog("kServiceOffline %s", service_info.fullname.c_str());
      RemoveDevice(service_info);
      ABDebugLog("device %s removed", service_info.fullname.c_str());
      return;
    }
  }

  void AddDevice(const BonjourBrowser::ServiceInfo& service_info) {
    aspl::DeviceParameters deviceParams;
    deviceParams.Name = service_info.name;
    deviceParams.SampleRate = SampleRate;
    deviceParams.ChannelCount = ChannelCount;
    deviceParams.EnableMixing = true;

    auto device = std::make_shared<aspl::Device>(context_, deviceParams);
    device->AddStreamWithControlsAsync(aspl::Direction::Output);
    auto raop_handler = std::make_shared<RaopHandler>(*device, service_info.ip);

    device->SetControlHandler(raop_handler);
    device->SetIOHandler(raop_handler);
    plugin_->AddDevice(device);
    devices_mapping_[service_info.fullname] = {device, service_info};
  }

  void RemoveDevice(const BonjourBrowser::ServiceInfo& service_info) {
    auto it = devices_mapping_.find(service_info.fullname);
    plugin_->RemoveDevice(it->second.device_);
    devices_mapping_.erase(it);
  }

 private:
  std::shared_ptr<aspl::Context> context_;
  std::shared_ptr<aspl::Plugin> plugin_;
  std::shared_ptr<aspl::Driver> driver_;
  std::map<std::string, DeviceInfo> devices_mapping_;
  std::mutex device_mapping_mutex_;
  BonjourBrowser browser_;
};

std::shared_ptr<aspl::Driver> CreateRaopDriver() {
  static DriverHelper helper;
  return helper.GetDriver();
}

}  // namespace

extern "C" void* AirBeamASPEntryPoint(CFAllocatorRef allocator,
                                      CFUUIDRef typeUUID) {
  if (!CFEqual(typeUUID, kAudioServerPlugInTypeUUID)) {
    return nullptr;
  }

  static std::shared_ptr<aspl::Driver> driver = CreateRaopDriver();

  return driver->GetReference();
}
