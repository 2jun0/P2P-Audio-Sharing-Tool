#ifndef AudioDeviceManagerWrapper_h
#define AudioDeviceManagerWrapper_h

#import "AudioDeviceObjC.h"
#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>

NS_ASSUME_NONNULL_BEGIN

@interface AudioDeviceManagerWrapper : NSObject

#if defined(__OBJC__) && defined(__cplusplus)
class AudioDeviceManager;
- (instancetype)initWithManager:(AudioDeviceManager *)manager;
#endif

- (NSArray<AudioDeviceObjC *> *)findAllAudioDevices;
- (AudioDeviceObjC *)findDefaultOutputDevice;

@end

NS_ASSUME_NONNULL_END

#endif /* AudioDeviceManagerWrapper_h */
