#ifndef cpp_audio_recorder_hpp
#define cpp_audio_recorder_hpp

#include <CoreAudio/AudioHardware.h>

class AudioLoopback
{
public:
    explicit AudioLoopback(AudioObjectID deviceID);
    ~AudioLoopback();

    // Start/stop IOProc. Returns true on success.
    bool start();
    void stop();

    OSStatus handleIOProc(const AudioBufferList *inInputData, AudioBufferList *outOutputData);

private:
    AudioObjectID deviceID = AudioObjectID(kAudioObjectUnknown);
    AudioDeviceIOProcID ioProcID = nullptr;
    bool recording = false;
};

OSStatus AudioLoopback_ioProc(AudioObjectID inDevice,
                              const AudioTimeStamp *inNow,
                              const AudioBufferList *inInputData,
                              const AudioTimeStamp *inInputTime,
                              AudioBufferList *outOutputData,
                              const AudioTimeStamp *outOutputTime,
                              void *inClientData);

#endif // cpp_audio_recorder_hpp
