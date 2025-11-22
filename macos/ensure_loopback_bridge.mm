#include "ensure_loopback_bridge.hpp"
#import "Audio-Swift.h"
#import "audio_loopback.hpp"
#include <CoreAudio/AudioHardwareBase.h>
#import <Foundation/Foundation.h>
#import <memory>

bool ensureMacosLoopbackDevice(const std::string &uid,
                               const std::string &name) {
  @autoreleasepool {
    NSString *uidNS = [NSString stringWithUTF8String:uid.c_str()];
    NSCAssert(uidNS != nil,
              @"ensureMacosLoopbackDevice: uid is not valid UTF-8");
    NSString *nameNS = [NSString stringWithUTF8String:name.c_str()];
    NSCAssert(nameNS != nil,
              @"ensureMacosLoopbackDevice: name is not valid UTF-8");

    NSNumber *deviceIDNS = [LoopbackHelper ensureLoopbackDeviceWithUid:uidNS
                                                                  name:nameNS];
    if (deviceIDNS == nil) {
      return false;
    }
    AudioObjectID deviceID = (AudioObjectID)[deviceIDNS unsignedIntValue];

    // Start audio loopback if not already started.
    static std::unique_ptr<AudioLoopback> audioLoopback;
    if (!audioLoopback) {
      audioLoopback = std::make_unique<AudioLoopback>(deviceID);
      if (!audioLoopback->start()) {
        audioLoopback.reset();
        return false;
      }
    }

    return true;
  }
}
