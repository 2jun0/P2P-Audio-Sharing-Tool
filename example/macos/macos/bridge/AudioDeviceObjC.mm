#import "AudioDeviceObjC.h"
#import "audio_device.hpp"
#include <string>

@implementation AudioDeviceObjC
@end

AudioDeviceObjC *audioDeviceObjCFrom(const AudioDevice &device) {
  AudioDeviceObjC *obj = [[AudioDeviceObjC alloc] init];
  obj.name = [NSString stringWithUTF8String:device.name.c_str()];
  obj.uid = [NSString stringWithUTF8String:device.uid.c_str()];
  obj.hasInput = device.hasInput;
  obj.hasOutput = device.hasOutput;
  obj.deviceID = (AudioObjectID)device.id;
  return obj;
}
