#pragma once

#include <CoreAudio/CoreAudio.h>
#include <MacTypes.h>

#include <functional>
#include <string>

namespace AirBeamCore {
namespace macos {

class VolumeObserver {
 public:
  using Callback = std::function<void(Float32)>;

  explicit VolumeObserver(AudioObjectID output_device_id, Callback callback);
  ~VolumeObserver();

  Float32 GetVolume();

  static OSStatus GetDefaultDeviceID(AudioObjectID& output_device_id);
  static OSStatus GetDefaultDeviceVolume(Float32& volume);
  static OSStatus FindAudioDeviceByUID(const std::string& uid,
                                       AudioDeviceID& device_id);

  void UpdateVolume();

 private:
  static OSStatus PropertyListenerCallback(
      AudioObjectID in_object_id, UInt32 in_number_addresses,
      const AudioObjectPropertyAddress* in_addresses, void* in_client_data);

  void StartObserve();
  void StopObserve();

  VolumeObserver& operator=(const VolumeObserver&) = delete;

 private:
  AudioObjectID output_device_id_;
  AudioObjectPropertyAddress volume_address_;
  Float32 volume_ = 0.0f;
  std::string err_msg_;
  Callback callback_;
};

}  // namespace macos
}  // namespace AirBeamCore
