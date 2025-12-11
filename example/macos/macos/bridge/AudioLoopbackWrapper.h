#ifndef AudioLoopbackWrapper_h
#define AudioLoopbackWrapper_h

#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

@interface AudioLoopbackWrapper : NSObject

- (instancetype)initWithDeviceID:(AudioObjectID)deviceID;
- (BOOL)start;
- (void)stop;

@end

#endif /* AudioLoopbackWrapper_h */
