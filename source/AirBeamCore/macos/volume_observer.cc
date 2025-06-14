#include "volume_observer.h"

#include <CoreFoundation/CoreFoundation.h>

namespace AirBeamCore {
namespace macos {

VolumeObserver::VolumeObserver(AudioObjectID output_device_id,
                               Callback callback)
    : output_device_id_(output_device_id), callback_(std::move(callback)) {
  StartObserve();
}

VolumeObserver::~VolumeObserver() { StopObserve(); }

Float32 VolumeObserver::GetVolume() { return volume_; }

OSStatus VolumeObserver::GetDefaultDeviceID(AudioObjectID& output_device_id) {
  UInt32 property_size = sizeof(output_device_id);
  AudioObjectPropertyAddress property_address = {
      kAudioHardwarePropertyDefaultOutputDevice,
      kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain};

  return AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address,
                                    0, nullptr, &property_size,
                                    &output_device_id);
}

OSStatus VolumeObserver::GetDefaultDeviceVolume(Float32& volume) {
  OSStatus status = noErr;
  AudioObjectID output_device_id = kAudioDeviceUnknown;
  status = GetDefaultDeviceID(output_device_id);
  if (status != noErr) {
    return status;
  }
  UInt32 dataSize = sizeof(volume);
  static const AudioObjectPropertyAddress volume_address = {
      kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput, 0};
  status = AudioObjectGetPropertyData(output_device_id, &volume_address, 0,
                                      nullptr, &dataSize, &volume);
  if (status != noErr) {
    return status;
  }

  return noErr;
}

OSStatus VolumeObserver::FindAudioDeviceByUID(const std::string& uid,
                                              AudioDeviceID& device_id) {
  UInt32 propertySize = 0;
  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain};

  OSStatus status = AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize);

  if (status != noErr) {
    return status;
  }

  UInt32 deviceCount = propertySize / sizeof(AudioDeviceID);

  std::vector<AudioDeviceID> deviceIDs(deviceCount);
  status =
      AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                                 NULL, &propertySize, deviceIDs.data());

  if (status != noErr) {
    return status;
  }

  for (UInt32 i = 0; i < deviceCount; i++) {
    CFStringRef deviceUID = NULL;
    propertySize = sizeof(deviceUID);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceUID;
    status = AudioObjectGetPropertyData(deviceIDs[i], &propertyAddress, 0, NULL,
                                        &propertySize, &deviceUID);

    if (status == noErr && deviceUID) {
      char uidBuffer[256];
      CFStringGetCString(deviceUID, uidBuffer, sizeof(uidBuffer),
                         kCFStringEncodingUTF8);
      CFRelease(deviceUID);

      if (uid == uidBuffer) {
        device_id = deviceIDs[i];
        return noErr;
      }
    }
  }

  return kAudioHardwareUnknownPropertyError;
}

void VolumeObserver::UpdateVolume() {
  Float32 current_volume = 0.0f;
  UInt32 dataSize = sizeof(current_volume);
  err_msg_ = "";
  OSStatus status =
      AudioObjectGetPropertyData(output_device_id_, &volume_address_, 0,
                                 nullptr, &dataSize, &current_volume);
  if (status != noErr) {
    err_msg_ = "Failed to get volume";
    return;
  }
  if (current_volume == volume_) {
    return;
  }
  volume_ = current_volume;
  callback_(volume_);
}

OSStatus VolumeObserver::PropertyListenerCallback(
    AudioObjectID in_object_id, UInt32 in_number_addresses,
    const AudioObjectPropertyAddress* in_addresses, void* in_client_data) {
  VolumeObserver& vb = *reinterpret_cast<VolumeObserver*>(in_client_data);
  for (UInt32 i = 0; i < in_number_addresses; i++) {
    if (in_addresses[i].mSelector == kAudioDevicePropertyVolumeScalar &&
        in_addresses[i].mScope == kAudioObjectPropertyScopeOutput &&
        in_addresses[i].mElement == kAudioObjectPropertyElementMain) {
      vb.UpdateVolume();
    }
  }

  return noErr;
}

void VolumeObserver::StartObserve() {
  OSStatus status = noErr;

  volume_address_ = {kAudioDevicePropertyVolumeScalar,
                     kAudioDevicePropertyScopeOutput, 0};
  status = AudioObjectAddPropertyListener(
      output_device_id_, &volume_address_,
      VolumeObserver::PropertyListenerCallback, static_cast<void*>(this));

  if (status != noErr) {
    err_msg_ = "Failed to add property listener";
    return;
  }

  UpdateVolume();
}

void VolumeObserver::StopObserve() {
  OSStatus status = AudioObjectRemovePropertyListener(
      output_device_id_, &volume_address_,
      VolumeObserver::PropertyListenerCallback, nullptr);
}

}  // namespace macos
}  // namespace AirBeamCore
