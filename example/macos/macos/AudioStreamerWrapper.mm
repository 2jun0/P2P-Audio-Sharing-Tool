#import "AudioStreamerWrapper.h"
#include "audio_streamer.hpp"

@implementation AudioStreamerWrapper {
  AudioStreamer *streamer;
}

- (instancetype)initWithPort:(int)port myId:(NSString *)myId {
  self = [super init];
  if (self) {
    streamer = new AudioStreamer(port, std::string([myId UTF8String]));
  }
  return self;
}

- (void)dealloc {
  if (streamer) {
    delete streamer;
    streamer = nullptr;
  }
}

- (void)start {
  streamer->start();
}

- (void)stop {
  streamer->stop();
}

- (void)startSendingTo:(NSString *)peerId
           inputDevice:(NSString *_Nullable)inputDevice {
  std::optional<AudioDevice> dev = std::nullopt;
  if (inputDevice) {
    dev = AudioDevice(std::string([inputDevice UTF8String]));
  }
  streamer->startSendingTo(std::string([peerId UTF8String]), dev);
}

- (void)stopSendingTo:(NSString *)peerId {
  streamer->stopSendingTo(std::string([peerId UTF8String]));
}

- (void)startReceivingFrom:(NSString *)peerId
              outputDevice:(NSString *_Nullable)outputDevice {
  std::optional<AudioDevice> dev = std::nullopt;
  if (outputDevice) {
    dev = AudioDevice(std::string([outputDevice UTF8String]));
  }
  streamer->startReceivingFrom(std::string([peerId UTF8String]), dev);
}

- (void)stopReceivingFrom:(NSString *)peerId {
  streamer->stopReceivingFrom(std::string([peerId UTF8String]));
}

- (void)changeOutputDeviceForPeer:(NSString *)peerId
                           device:(NSString *_Nullable)device {
  std::optional<AudioDevice> dev = std::nullopt;
  if (device) {
    dev = AudioDevice(std::string([device UTF8String]));
  }
  streamer->changeOutputDevice(std::string([peerId UTF8String]), dev);
}

- (void)changeInputDeviceForPeer:(NSString *)peerId
                          device:(NSString *_Nullable)device {
  std::optional<AudioDevice> dev = std::nullopt;
  if (device) {
    dev = AudioDevice(std::string([device UTF8String]));
  }
  streamer->changeInputDevice(std::string([peerId UTF8String]), dev);
}

- (NSArray<NSString *> *)getPeerIds {
  std::vector<std::string> peers = streamer->getPeerIds();
  NSMutableArray *arr = [NSMutableArray arrayWithCapacity:peers.size()];

  for (auto &p : peers) {
    [arr addObject:[NSString stringWithUTF8String:p.c_str()]];
  }
  return arr;
}

@end
