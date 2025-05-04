// Copyright (c) libASPL authors
// Licensed under MIT

#include "aspl/Driver.hpp"

#include <CoreAudio/AudioServerPlugIn.h>
#include <CoreAudio/CoreAudio.h>
#include <arpa/inet.h>
#include <fmt/core.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <chrono>
#include <memory>
#include <thread>

#include "aspl/ControlRequestHandler.hpp"
#include "aspl/Device.hpp"
#include "aspl/IORequestHandler.hpp"
#include "aspl/Plugin.hpp"
#include "helper/bonjour_browser.hpp"
#include "helper/volume_observer.hpp"
#include "raop/codec.h"
#include "raop/fifo.h"
#include "raop/raop.h"

namespace {
// Stream format.
constexpr UInt32 SampleRate = 44100;
constexpr UInt32 ChannelCount = 2;

const size_t kFiFOCapacity = 1024 * 1024 * 1024;

// Control and I/O request handler.
class RaopHandler : public aspl::ControlRequestHandler,
                    public aspl::IORequestHandler {
  public:
    explicit RaopHandler(aspl::Device& device, const std::string& ip)
        : raop_(std::make_shared<Raop>(ip)), fifo_(kFiFOCapacity),
          device_(device) {
        auto volume_control =
            device_.GetVolumeControlByIndex(kAudioObjectPropertyScopeOutput, 0);
        volume_control->SetScalarValue(0.5);
    }

    // TODO(cattchen) 当StartIO时才连接client，以及监听音量
    OSStatus OnStartIO() override {
        Prepare();

        return kAudioHardwareNoError;
    }

    // Invoked on control thread after last I/O request.
    void OnStopIO() override {}

    // Invoked on realtime I/O thread to write mixed data from clients.
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

        // 监听音量变化
        (new std::thread([=]() {
            auto device_uid = device_.GetDeviceUID();

            AudioObjectID output_device_id = 0;
            while (true) {
                OSStatus status = VolumeObserver::FindAudioDeviceByUID(
                    device_uid, output_device_id);

                // syslog(LOG_NOTICE, "RaopDriver FindAudioDeviceByUID %d %d",
                // status,
                //        output_device_id);
                if (status == noErr) {
                    break;
                }
                if (status == kAudioHardwareUnknownPropertyError) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                return;
            }
            // syslog(LOG_NOTICE, "RaopDriver output_device_id %d",
            // output_device_id);

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
        int64_t last_update_;
    };

  public:
    DriverHelper()
        : context_(std::make_shared<aspl::Context>()),
          plugin_(std::make_shared<aspl::Plugin>(context_)),
          driver_(std::make_shared<aspl::Driver>(context_, plugin_)),
          devices_mapping_() {}

    std::shared_ptr<aspl::Driver> GetDriver() { return driver_; }

    void StartLoop() {
        (new std::thread([=]() {
            while (true) {
                browser_.startBrowse(
                    "_raop._tcp",
                    [=](const std::string& name, const std::string& fullname,
                        const std::string& ip, uint16_t port) {
                        AddOrUpdateDevice(fullname, name, ip, port);
                    });
                RemoveExpiredDevices();
                std::this_thread::sleep_for(std::chrono::seconds(3));
                browser_.stop();
            };
        }))->detach();
    }

  private:
    void AddOrUpdateDevice(const std::string& fullname, const std::string& name,
                           const std::string& ip, uint16_t port) {
        using namespace std::chrono;
        auto now = duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

        // 如果设备已经存在，则更新ttl时间戳
        auto it = devices_mapping_.find(fullname);
        if (it != devices_mapping_.end()) {
            (*it).second.last_update_ = now;
            return;
        }
        // 如果设备不存在，则创建设备
        aspl::DeviceParameters deviceParams;
        deviceParams.Name = name;
        deviceParams.SampleRate = SampleRate;
        deviceParams.ChannelCount = ChannelCount;
        deviceParams.EnableMixing = true;
        // deviceParams.IconURL = "";

        auto device = std::make_shared<aspl::Device>(context_, deviceParams);
        device->AddStreamWithControlsAsync(aspl::Direction::Output);
        auto raop_handler = std::make_shared<RaopHandler>(*device, ip);

        device->SetControlHandler(raop_handler);
        device->SetIOHandler(raop_handler);
        plugin_->AddDevice(device);

        devices_mapping_[fullname] = {device, now};
    }

    // 移除过期的设备
    void RemoveExpiredDevices() {
        constexpr int64_t kExpireTTL = 10;

        using namespace std::chrono;
        auto now = duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
        for (auto it = devices_mapping_.begin(); it != devices_mapping_.end();
             it++) {
            auto& device_info = it->second;
            auto device_last_update = device_info.last_update_;
            if (device_last_update + kExpireTTL < now) {
                plugin_->RemoveDevice(device_info.device_);
                it = devices_mapping_.erase(it);
                it--;
                continue;
            }
        }
    }

  private:
    std::shared_ptr<aspl::Context> context_;
    std::shared_ptr<aspl::Plugin> plugin_;
    std::shared_ptr<aspl::Driver> driver_;
    std::map<std::string, DeviceInfo> devices_mapping_;
    BonjourBrowser browser_;
};

std::shared_ptr<aspl::Driver> CreateRaopDriver() {
    static DriverHelper helper;
    helper.StartLoop();
    return helper.GetDriver();
}

} // namespace

extern "C" void* RaopDeviceEntryPoint(CFAllocatorRef allocator,
                                      CFUUIDRef typeUUID) {
    // syslog(LOG_NOTICE, "RaopDriver RaopDeviceEntryPoint");

    // The UUID of the plug-in type (443ABAB8-E7B3-491A-B985-BEB9187030DB).
    if (!CFEqual(typeUUID, kAudioServerPlugInTypeUUID)) {
        return nullptr;
    }

    // Store shared pointer to the driver to keep it alive.
    static std::shared_ptr<aspl::Driver> driver = CreateRaopDriver();

    return driver->GetReference();
}
