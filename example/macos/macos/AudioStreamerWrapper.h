#ifndef AudioStreamerWrapper_h
#define AudioStreamerWrapper_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AudioStreamerWrapper : NSObject

- (instancetype)initWithPort:(int)port myId:(NSString *)myId;

- (void)start;
- (void)stop;

// User API
- (void)startSendingTo:(NSString *)peerId inputDevice:(nullable NSString *)inputDevice;
- (void)stopSendingTo:(NSString *)peerId;

- (void)startReceivingFrom:(NSString *)peerId outputDevice:(nullable NSString *)outputDevice;
- (void)stopReceivingFrom:(NSString *)peerId;

- (void)changeOutputDeviceForPeer:(NSString *)peerId device:(nullable NSString *)device;
- (void)changeInputDeviceForPeer:(NSString *)peerId device:(nullable NSString *)device;

- (NSArray<NSString *> *)getPeerIds;

@end

NS_ASSUME_NONNULL_END

#endif /* AudioStreamerWrapper_h */
