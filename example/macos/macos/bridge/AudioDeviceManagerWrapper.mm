#import "AudioDeviceManagerWrapper.h"
#import "AudioDeviceObjC.h"

#include "audio_device.hpp"
#include "mac_audio_device_manager.hpp"
#include <vector>

@implementation AudioDeviceManagerWrapper {
  AudioDeviceManager *_cpp;
}

- (instancetype)initWithManager:(AudioDeviceManager *)manager {
  self = [super init];
  if (self) {
    _cpp = manager;
  }
  return self;
}

- (NSArray<AudioDeviceObjC *> *)findAllAudioDevices {
  std::vector<AudioDevice> devices = _cpp->findAllAudioDevices();
  NSMutableArray<AudioDeviceObjC *> *result =
      [NSMutableArray arrayWithCapacity:devices.size()];

  for (const auto &d : devices) {
    AudioDeviceObjC *obj = audioDeviceObjCFrom(d);
    [result addObject:obj];
  }
  return result;
}

- (AudioDeviceObjC *)findDefaultOutputDevice {
  AudioDevice device = _cpp->findDefaultOutputDevice();
  return audioDeviceObjCFrom(device);
}

@end
