#ifndef AudioStreamerWrapper_h
#define AudioStreamerWrapper_h

#import <Foundation/Foundation.h>
#import <CoreAudio/AudioHardware.h>
#import "AudioDeviceManagerWrapper.h"

NS_ASSUME_NONNULL_BEGIN

@interface AudioStreamerWrapper : NSObject

- (instancetype)initWithPort:(int)port myId:(NSString *)myId;

- (void)start;
- (void)stop;

// User API
- (void)startSendingTo:(NSString *)peerId inputDeviceUID:(NSString *_Nullable)deviceUID;
- (void)stopSendingTo:(NSString *)peerId;
- (void)startReceivingFrom:(NSString *)peerId outputDeviceUID:(NSString *_Nullable)deviceUID;
- (void)stopReceivingFrom:(NSString *)peerId;
- (void)changeOutputDeviceFor:(NSString *)peerId outputDeviceUID:(NSString *_Nullable)deviceUID;
- (void)changeInputDeviceFor:(NSString *)peerId inputDeviceUID:(NSString *_Nullable)deviceUID;
- (NSArray<NSString *> *)getPeers;
- (AudioDeviceManagerWrapper *)getAudioDeviceManager;

@end

NS_ASSUME_NONNULL_END

#endif /* AudioStreamerWrapper_h */
