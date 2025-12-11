#ifndef AudioDeviceC_h
#define AudioDeviceC_h

#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>

NS_ASSUME_NONNULL_BEGIN

@interface AudioDeviceObjC : NSObject

@property(nonatomic, strong) NSString *name;
@property(nonatomic, strong) NSString *uid;
@property(nonatomic) BOOL hasInput;
@property(nonatomic) BOOL hasOutput;
@property(nonatomic) AudioObjectID deviceID;

@end

#if defined(__OBJC__) && defined(__cplusplus)
struct AudioDevice;
AudioDeviceObjC *audioDeviceObjCFrom(const AudioDevice &device);
#endif

NS_ASSUME_NONNULL_END

#endif /* AudioDeviceC_h */
