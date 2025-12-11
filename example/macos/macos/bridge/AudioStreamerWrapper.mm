#import "AudioStreamerWrapper.h"
#include "audio_streamer.hpp"

@implementation AudioStreamerWrapper {
  AudioStreamer *streamer;
}

- (instancetype)initWithPort:(int)port myId:(NSString *)myId {
  self = [super init];
  if (self) {
    streamer = new AudioStreamer(port, [myId UTF8String]);
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
           inputDevice:(NSString *_Nullable)deviceUID {
  std::string peerIdStr = std::string([peerId UTF8String]);
  std::optional<std::string> inputDeviceOpt =
      deviceUID ? std::optional<std::string>([deviceUID UTF8String])
                : std::nullopt;

  streamer->startSendingTo(peerIdStr, inputDeviceOpt);
}

- (void)stopSendingTo:(NSString *)peerId {
  streamer->stopSendingTo(std::string([peerId UTF8String]));
}

- (void)startReceivingFrom:(NSString *)peerId
              outputDevice:(NSString *_Nullable)deviceUID {
  std::string peerIdStr = std::string([peerId UTF8String]);
  std::optional<std::string> outputDeviceOpt =
      deviceUID ? std::optional<std::string>([deviceUID UTF8String])
                : std::nullopt;

  streamer->startReceivingFrom(peerIdStr, outputDeviceOpt);
}

- (void)stopReceivingFrom:(NSString *)peerId {
  streamer->stopReceivingFrom(std::string([peerId UTF8String]));
}

- (void)changeOutputDeviceFor:(NSString *)peerId
                 outputDevice:(NSString *_Nullable)deviceUID {
  std::string peerIdStr = std::string([peerId UTF8String]);
  std::optional<std::string> outputDeviceOpt =
      deviceUID ? std::optional<std::string>([deviceUID UTF8String])
                : std::nullopt;

  streamer->changeOutputDevice(peerIdStr, outputDeviceOpt);
}

- (void)changeInputDeviceFor:(NSString *)peerId
                 inputDevice:(NSString *_Nullable)deviceUID {
  std::string peerIdStr = std::string([peerId UTF8String]);
  std::optional<std::string> inputDeviceOpt =
      deviceUID ? std::optional<std::string>([deviceUID UTF8String])
                : std::nullopt;

  streamer->changeInputDevice(peerIdStr, inputDeviceOpt);
}

- (NSArray<NSString *> *)getPeerIds {
  std::vector<std::string> peers = streamer->getPeerIds();
  NSMutableArray *arr = [NSMutableArray arrayWithCapacity:peers.size()];

  for (auto &p : peers) {
    [arr addObject:[NSString stringWithUTF8String:p.c_str()]];
  }
  return arr;
}

- (AudioDeviceManagerWrapper *)getAudioDeviceManager {
  AudioDeviceManager &manager = streamer->getAudioDeviceManager();
  AudioDeviceManagerWrapper *wrapper =
      [[AudioDeviceManagerWrapper alloc] initWithManager:&manager];
  return wrapper;
}

@end
