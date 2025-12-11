#import "AudioLoopbackWrapper.h"
#import "audio_loopback.hpp"

@interface AudioLoopbackWrapper () {
  AudioLoopback *_cpp;
}
@end

@implementation AudioLoopbackWrapper

- (instancetype)initWithDeviceID:(AudioObjectID)deviceID {
  self = [super init];
  if (self) {
    _cpp = new AudioLoopback(deviceID);
  }
  return self;
}

- (BOOL)start {
  if (_cpp)
    return _cpp->start();
  return NO;
}

- (void)stop {
  if (_cpp)
    _cpp->stop();
}

- (void)dealloc {
  if (_cpp) {
    delete _cpp;
    _cpp = nullptr;
  }
}

@end
